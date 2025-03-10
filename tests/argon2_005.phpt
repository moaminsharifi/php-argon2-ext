--TEST--
Tests Argon2i hash and verify with options
--FILE--
<?php
$password = 'password';
$salt = random_bytes(32);

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 17 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 20 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 't_cost' => 1 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 't_cost' => 2 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 't_cost' => 3 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 't_cost' => 3 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 16, 't_cost' => 1 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 18, 't_cost' => 2 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 20, 't_cost' => 3 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 17, 't_cost' => 1, 'threads' => 1 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 30, 't_cost' => 2, 'threads' => 2 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));

$hash = argon2_hash($password, $salt, HASH_ARGON2I, [ 'm_cost' => 30, 't_cost' => 3, 'threads' => 3 ]);
var_dump(argon2_verify($password, $hash));
var_dump(argon2_verify('wrongpassword', $hash));
--EXPECT--
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)