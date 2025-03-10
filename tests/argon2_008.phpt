--TEST--
Verifies argon2_get_info returns correct data
--FILE--
<?php
$salt = random_bytes(32);

var_dump(argon2_get_info(argon2_hash('test', $salt, HASH_ARGON2ID)));
var_dump(argon2_get_info(argon2_hash('test', $salt, HASH_ARGON2I)));
var_dump(argon2_get_info(argon2_hash('test', $salt, HASH_ARGON2D, [
    'm_cost' => 1<<15,
    't_cost' => 4,
    'threads' => 2
])));
--EXPECT--
array(2) {
  ["algorithm"]=>
  string(8) "argon2id"
  ["options"]=>
  array(3) {
    ["m_cost"]=>
    int(65536)
    ["t_cost"]=>
    int(3)
    ["threads"]=>
    int(1)
  }
}
array(2) {
  ["algorithm"]=>
  string(7) "argon2i"
  ["options"]=>
  array(3) {
    ["m_cost"]=>
    int(65536)
    ["t_cost"]=>
    int(3)
    ["threads"]=>
    int(1)
  }
}
array(2) {
  ["algorithm"]=>
  string(7) "argon2d"
  ["options"]=>
  array(3) {
    ["m_cost"]=>
    int(32768)
    ["t_cost"]=>
    int(4)
    ["threads"]=>
    int(2)
  }
}