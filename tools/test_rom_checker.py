#!/usr/bin/env python3
"""Synthetic byte-order checks only; no game data."""
import unittest
from check_rom import canonical

class ByteOrderTests(unittest.TestCase):
    def test_three_orders(self):
        big = bytes.fromhex("80371240000102030405060708090a0b")
        half = bytearray(len(big))
        half[::2], half[1::2] = big[1::2], big[::2]
        little = bytearray(len(big))
        for i in range(4):
            little[i::4] = big[3-i::4]
        for data in (big, half, little):
            with self.subTest(header=data[:4].hex()):
                self.assertEqual(bytes(canonical(data)[0]), big)

    def test_bad_header(self):
        with self.assertRaises(ValueError):
            canonical(b"BAD!")

    def test_misaligned_swap(self):
        with self.assertRaises(ValueError):
            canonical(bytes.fromhex("37804012") + b"x")
        with self.assertRaises(ValueError):
            canonical(bytes.fromhex("40123780") + b"xx")

if __name__ == "__main__":
    unittest.main()
