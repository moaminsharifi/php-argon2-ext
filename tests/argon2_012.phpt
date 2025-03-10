--TEST--
Tests Argon2 raw hash functionality
--FILE--
<?php
// Test 1: Basic raw hash generation and verification
$password = 'password';
$salt = random_bytes(32);

// Generate raw hash
$raw_hash = argon2_hash_raw($password, $salt);

// Generate encoded hash with the same parameters
$encoded_hash = argon2_hash($password, $salt);

// Verify the raw hash works for verification
$raw_hash_hex = bin2hex($raw_hash);
echo "Raw hash length: " . strlen($raw_hash) . " bytes\n";
echo "Raw hash is binary: " . (ctype_print($raw_hash) ? 'false' : 'true') . "\n";

// Test 2: Compare with regular hash function in raw mode
$regular_raw_hash = argon2_hash($password, $salt, HASH_ARGON2ID, [], true);
echo "Hashes match: " . ($raw_hash === $regular_raw_hash ? 'true' : 'false') . "\n";

// Test 3: Custom hash length
$custom_len = 64;
$custom_raw_hash = argon2_hash_raw($password, $salt, HASH_ARGON2ID, [], $custom_len);
echo "Custom length hash size: " . strlen($custom_raw_hash) . " bytes\n";

// Test 4: Different algorithms
$id_hash = argon2_hash_raw($password, $salt, HASH_ARGON2ID);
$i_hash = argon2_hash_raw($password, $salt, HASH_ARGON2I);
$d_hash = argon2_hash_raw($password, $salt, HASH_ARGON2D);
echo "Different algorithms produce different hashes: " .
     (($id_hash !== $i_hash && $id_hash !== $d_hash && $i_hash !== $d_hash) ? 'true' : 'false') . "\n";

// Test 5: Verify that verification still works with encoded hash
echo "Verification with encoded hash: " . (argon2_verify($password, $encoded_hash) ? 'true' : 'false') . "\n";
--EXPECT--
Raw hash length: 32 bytes
Raw hash is binary: true
Hashes match: true
Custom length hash size: 64 bytes
Different algorithms produce different hashes: true
Verification with encoded hash: true