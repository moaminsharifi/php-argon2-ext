--TEST--
Tests Argon2 hash and verify with salt parameter
--FILE--
<?php
$salt = random_bytes(32);
$hash = argon2_hash('password', $salt);
var_dump(argon2_verify('password', $hash));
--EXPECT--
bool(true)