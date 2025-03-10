#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/standard/base64.h"
#include "ext/standard/php_random.h"

#include "ext/argon2/include/argon2.h"
#include "php_argon2.h"

// Zend Argument information
ZEND_BEGIN_ARG_INFO_EX(arginfo_argon2_hash, 0, 0, 3)
	ZEND_ARG_INFO(0, password)
	ZEND_ARG_INFO(0, salt)
	ZEND_ARG_INFO(0, algorithm)
	ZEND_ARG_INFO(0, options)
	ZEND_ARG_INFO(0, raw)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_argon2_verify, 0, 0, 2)
	ZEND_ARG_INFO(0, password)
	ZEND_ARG_INFO(0, hash)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_argon2_get_info, 0, 0, 1)
	ZEND_ARG_INFO(0, hash)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_argon2_hash_raw, 0, 0, 2)
    ZEND_ARG_INFO(0, password)
    ZEND_ARG_INFO(0, salt)
    ZEND_ARG_INFO(0, algorithm)
    ZEND_ARG_INFO(0, options)
    ZEND_ARG_INFO(0, hash_len)
ZEND_END_ARG_INFO()




static int php_password_salt_to64(const char *str, const size_t str_len, const size_t out_len, char *ret) /* {{{ */
{
	size_t pos = 0;
	zend_string *buffer;
	if ((int) str_len < 0) {
		return FAILURE;
	}
	buffer = php_base64_encode((unsigned char*) str, str_len);
	if (ZSTR_LEN(buffer) < out_len) {
		/* Too short of an encoded string generated */
		zend_string_release(buffer);
		return FAILURE;
	}
	for (pos = 0; pos < out_len; pos++) {
		if (ZSTR_VAL(buffer)[pos] == '+') {
			ret[pos] = '.';
		} else if (ZSTR_VAL(buffer)[pos] == '=') {
			zend_string_free(buffer);
			return FAILURE;
		} else {
			ret[pos] = ZSTR_VAL(buffer)[pos];
		}
	}
	zend_string_free(buffer);
	return SUCCESS;
}
/* }}} */

static int php_password_make_salt(size_t length, char *ret) /* {{{ */
{
	size_t raw_length;
	char *buffer;
	char *result;

	if (length > (INT_MAX / 3)) {
		php_error_docref(NULL, E_WARNING, "Length is too large to safely generate");
		return FAILURE;
	}

	raw_length = length * 3 / 4 + 1;

	buffer = (char *) safe_emalloc(raw_length, 1, 1);

	if (FAILURE == php_random_bytes_silent(buffer, raw_length)) {
		php_error_docref(NULL, E_WARNING, "Unable to generate salt");
		efree(buffer);
		return FAILURE;
	}

	result = safe_emalloc(length, 1, 1);
	if (php_password_salt_to64(buffer, raw_length, length, result) == FAILURE) {
		php_error_docref(NULL, E_WARNING, "Generated salt too short");
		efree(buffer);
		efree(result);
		return FAILURE;
	}
	memcpy(ret, result, length);
	efree(result);
	efree(buffer);
	ret[length] = 0;
	return SUCCESS;
}
/* }}} */

/* {{{ proto string argon2_hash(string password, string salt, int algorithm, array options)
Generates an argon2 hash */
PHP_FUNCTION(argon2_hash)
{
    // Argon2 Options
    uint32_t t_cost = ARGON2_TIME_COST;
    uint32_t m_cost = ARGON2_MEMORY_COST;
    uint32_t threads = ARGON2_THREADS;
    uint32_t lanes;
    uint32_t out_len = 32;
    argon2_type type = EXT_HASH_ARGON2ID;

    size_t salt_len;
    size_t password_len;
    size_t encoded_len;

    char *salt;
    char *password;
    char *encoded_result;

    int result;
    long argon2_type = -1;

    zend_bool raw = 0;

    zval *option_buffer;
    HashTable *options = 0;

    ZEND_PARSE_PARAMETERS_START(2, 5)
        Z_PARAM_STRING(password, password_len)
        Z_PARAM_STRING(salt, salt_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(argon2_type)
        Z_PARAM_ARRAY_HT(options)
        Z_PARAM_BOOL(raw);
    ZEND_PARSE_PARAMETERS_END();

    // Determine the m_cost if it was passed via options
    if (options && (option_buffer = zend_hash_str_find(options, "m_cost", sizeof("m_cost")-1)) != NULL) {
        m_cost = zval_get_long(option_buffer);
    }

    if (m_cost > ARGON2_MAX_MEMORY || m_cost == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Memory cost is not valid", 0);
        RETURN_FALSE;
    }

    // Determine the t_cost if it was passed via options
    if (options && (option_buffer = zend_hash_str_find(options, "t_cost", sizeof("t_cost")-1)) != NULL) {
        t_cost = zval_get_long(option_buffer);
    }

    if (t_cost > ARGON2_MAX_TIME || t_cost == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Time cost is not valid", 0);
        RETURN_FALSE;
    }

    // Determine the parallelism degree if it was passed via options
    if (options && (option_buffer = zend_hash_str_find(options, "threads", sizeof("threads")-1)) != NULL) {
        threads = zval_get_long(option_buffer);
    }

    if (threads > ARGON2_MAX_LANES || threads == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Number of threads is not valid", 0);
        RETURN_FALSE;
    }

    lanes = threads;

    // Sanity check the password for non-zero length
    if (password_len == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Password must be provided", 0);
        RETURN_FALSE;
    }

    // Sanity check the salt for non-zero length
    if (salt_len == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Salt must be provided", 0);
        RETURN_FALSE;
    }

    // Determine the Algorithm type
    if (argon2_type == EXT_HASH_ARGON2ID || argon2_type == -1) {
        type = EXT_HASH_ARGON2ID;
    } else if (argon2_type == EXT_HASH_ARGON2I) {
        type = EXT_HASH_ARGON2I;
    } else if (argon2_type == EXT_HASH_ARGON2D) {
        type = EXT_HASH_ARGON2D;
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Algorithm must be one of `HASH_ARGON2ID, HASH_ARGON2I, HASH_ARGON2D`", 0);
        RETURN_FALSE;
    }

    // Determine the encoded length
    encoded_len = argon2_encodedlen(
        t_cost,
        m_cost,
        threads,
        (uint32_t)salt_len,
        out_len,
        type
    );

    // Allocate the size of encoded, and out
    zend_string *out = zend_string_alloc(out_len, 0);
    zend_string *encoded = zend_string_alloc(encoded_len, 0);

    // Generate the argon2_hash
    result = argon2_hash(
        t_cost,
        m_cost,
        threads,
        password,
        password_len,
        salt,
        salt_len,
        out->val,
        out_len,
        encoded->val,
        encoded_len,
        type,
        ARGON2_VERSION_NUMBER
    );

    // If the hash wasn't generated, throw an exception
    if (result != ARGON2_OK) {
        zend_string_efree(encoded);
        zend_string_efree(out);
        php_error_docref(NULL, E_WARNING, argon2_error_message(result));
        RETURN_FALSE;
    }

    if (raw == 0) {
        zend_string_efree(out);
        RETURN_STR(encoded);
    } else {
        zend_string_efree(encoded);
        RETURN_STR(out);
    }
}
/* }}} */

/* {{{ proto string argon2_hash_raw(string password, string salt, int algorithm, array options, int hash_len)
Generates a raw argon2 hash */
PHP_FUNCTION(argon2_hash_raw)
{
    // Argon2 Options
    uint32_t t_cost = ARGON2_TIME_COST;
    uint32_t m_cost = ARGON2_MEMORY_COST;
    uint32_t threads = ARGON2_THREADS;
    uint32_t lanes;
    uint32_t out_len = 32;
    argon2_type type = EXT_HASH_ARGON2ID;

    size_t salt_len;
    size_t password_len;

    char *salt;
    char *password;

    int result;
    long argon2_type = -1;
    zend_long hash_len = 32;

    zval *option_buffer;
    HashTable *options = 0;

    ZEND_PARSE_PARAMETERS_START(2, 5)
        Z_PARAM_STRING(password, password_len)
        Z_PARAM_STRING(salt, salt_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(argon2_type)
        Z_PARAM_ARRAY_HT(options)
        Z_PARAM_LONG(hash_len)
    ZEND_PARSE_PARAMETERS_END();

    // Use provided hash_len if specified
    if (hash_len > 0) {
        out_len = (uint32_t)hash_len;
    }

    // Determine the m_cost if it was passed via options
    if (options && (option_buffer = zend_hash_str_find(options, "m_cost", sizeof("m_cost")-1)) != NULL) {
        m_cost = zval_get_long(option_buffer);
    }

    if (m_cost > ARGON2_MAX_MEMORY || m_cost == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Memory cost is not valid", 0);
        RETURN_FALSE;
    }

    // Determine the t_cost if it was passed via options
    if (options && (option_buffer = zend_hash_str_find(options, "t_cost", sizeof("t_cost")-1)) != NULL) {
        t_cost = zval_get_long(option_buffer);
    }

    if (t_cost > ARGON2_MAX_TIME || t_cost == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Time cost is not valid", 0);
        RETURN_FALSE;
    }

    // Determine the parallelism degree if it was passed via options
    if (options && (option_buffer = zend_hash_str_find(options, "threads", sizeof("threads")-1)) != NULL) {
        threads = zval_get_long(option_buffer);
    }

    if (threads > ARGON2_MAX_LANES || threads == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Number of threads is not valid", 0);
        RETURN_FALSE;
    }

    lanes = threads;

    // Sanity check the password for non-zero length
    if (password_len == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Password must be provided", 0);
        RETURN_FALSE;
    }

    // Sanity check the salt for non-zero length
    if (salt_len == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Salt must be provided", 0);
        RETURN_FALSE;
    }

    // Determine the Algorithm type
    if (argon2_type == EXT_HASH_ARGON2ID || argon2_type == -1) {
        type = EXT_HASH_ARGON2ID;
    } else if (argon2_type == EXT_HASH_ARGON2I) {
        type = EXT_HASH_ARGON2I;
    } else if (argon2_type == EXT_HASH_ARGON2D) {
        type = EXT_HASH_ARGON2D;
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Algorithm must be one of `HASH_ARGON2ID, HASH_ARGON2I, HASH_ARGON2D`", 0);
        RETURN_FALSE;
    }

    // Allocate the raw hash output buffer
    zend_string *out = zend_string_alloc(out_len, 0);

    // Generate the raw argon2_hash
    result = argon2_hash(
        t_cost,
        m_cost,
        threads,
        password,
        password_len,
        salt,
        salt_len,
        (uint8_t *)out->val,
        out_len,
        NULL,  // No encoded output
        0,     // No encoded output length
        type,
        ARGON2_VERSION_NUMBER
    );

    // If the hash wasn't generated, throw an exception
    if (result != ARGON2_OK) {
        zend_string_efree(out);
        php_error_docref(NULL, E_WARNING, argon2_error_message(result));
        RETURN_FALSE;
    }

    RETURN_STR(out);
}
/* }}} */

/* {{{ proto string argon2_verify(string password, string hash)
Generates an argon2 hash */
PHP_FUNCTION(argon2_verify)
{
	// Argon2 Options
	argon2_type type = EXT_HASH_ARGON2ID; 	// Default to Argon2_id

	size_t password_len;
	size_t encoded_len;

	char *password;
	char *encoded;

	int result;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STRING(password, password_len)
		Z_PARAM_STRING(encoded, encoded_len)
	ZEND_PARSE_PARAMETERS_END();
 
	// Determine which algorithm is used
	if (strstr(encoded, "argon2id")) {
		type = EXT_HASH_ARGON2ID;
	} else if (strstr(encoded, "argon2d")) {
		type = EXT_HASH_ARGON2D;
	} else if (strstr(encoded, "argon2i")) {
		type = EXT_HASH_ARGON2I;
	} else {
		php_error_docref(NULL, E_WARNING, "Invalid Argon2 hash");
		RETURN_FALSE;
	}

	result = argon2_verify(encoded, password, password_len, type);

	// If verification failed just return false
	if (result != ARGON2_OK) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string argon2_get_info(string hash)
Returns information about a argon2 hash */
PHP_FUNCTION(argon2_get_info)
{
	char *hash;
	char *algo;
	size_t hash_len;

	zval options;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STRING(hash, hash_len)
	ZEND_PARSE_PARAMETERS_END();

	if (strstr(hash, "argon2id")) {
		algo = "argon2id";
	} else if (strstr(hash, "argon2d")) {
		algo = "argon2d";
	} else if (strstr(hash, "argon2i")) {
		algo = "argon2i";
	} else {
		php_error_docref(NULL, E_WARNING, "Invalid Argon2 hash");
		RETURN_FALSE;
	}

	array_init(&options);

	zend_long v = 0;
	zend_long m_cost = ARGON2_MEMORY_COST;
	zend_long t_cost = ARGON2_TIME_COST;
	zend_long threads = ARGON2_THREADS;

	sscanf(hash, "$%*[argon2id]$v=%*ld$m=" ZEND_LONG_FMT ",t=" ZEND_LONG_FMT ",p=" ZEND_LONG_FMT, &m_cost, &t_cost, &threads);
	add_assoc_long(&options, "m_cost", m_cost);
	add_assoc_long(&options, "t_cost", t_cost);
	add_assoc_long(&options, "threads", threads);

	array_init(return_value);

	add_assoc_string(return_value, "algorithm", algo);
	add_assoc_zval(return_value, "options", &options);
}
/* }}} */

/* {{{ argon2_functions[]
 */
const zend_function_entry argon2_functions[] = {
	PHP_FE(argon2_hash, arginfo_argon2_hash)
    PHP_FE(argon2_hash_raw, arginfo_argon2_hash_raw)
	PHP_FE(argon2_verify, arginfo_argon2_verify)
	PHP_FE(argon2_get_info, arginfo_argon2_get_info)
	PHP_FE_END
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(argon2)
{
	// Create contants for Argon2
	REGISTER_LONG_CONSTANT("HASH_ARGON2D", EXT_HASH_ARGON2D, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("HASH_ARGON2I", EXT_HASH_ARGON2I, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("HASH_ARGON2ID", EXT_HASH_ARGON2ID, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("HASH_ARGON2", EXT_HASH_ARGON2, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(argon2)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Argon2 support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(argon2)
{
#if defined(COMPILE_DL_ARGON2) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* {{{ argon2_module_entry
 */
zend_module_entry argon2_module_entry = {
	STANDARD_MODULE_HEADER,
	"argon2",
	argon2_functions,
	PHP_MINIT(argon2),
	NULL,
	PHP_RINIT(argon2),
	NULL,
	PHP_MINFO(argon2),
	PHP_ARGON2_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_ARGON2
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(argon2)
#endif
