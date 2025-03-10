--TEST--
Tests Argon2 raw output
--FILE--
<?php
$salt = random_bytes(32);
var_dump(strlen(argon2_hash("test", $salt, HASH_ARGON2ID, ['m_cost' => 1<<12, 't_cost' => 3, 'threads' => 1], true)));
var_dump(strlen(argon2_hash("test", $salt, HASH_ARGON2D, ['m_cost' => 1<<12, 't_cost' => 3, 'threads' => 1], true)));
var_dump(strlen(argon2_hash("test", $salt, HASH_ARGON2I, ['m_cost' => 1<<12, 't_cost' => 3, 'threads' => 1], true)));
--EXPECT--
int(32)
int(32)
int(32)