package main

import (
	"fmt"
)

// u mod v
func inverse(u, v int) int {
	u3 := u
	v3 := v
	u1 := 1
	v1 := 0

	for {
		if v3 <= 0 {
			break
		}
		q := u3 / v3
		u1, v1 = v1, u1-v1*q
		u3, v3 = v3, u3-v3*q
	}
	for {
		if u1 < 0 {
			u1 = u1 + v
		} else {
			break
		}
	}
	return u1
}

// x^c Mod n
func squareAndMultipy(x, c, n int) int {
	var z int
	z = 1

	var seq []int

	for {
		if c <= 0 {
			break
		}
		if (c & 1) != 0 {
			seq = append(seq, 1)
		} else {
			seq = append(seq, 0)
		}
		c = c / 2
	}
	for i := len(seq) - 1; i >= 0; i-- {
		z = (z * z) % n
		if seq[i] == 1 {
			z = (z * x) % n
		}
	}
	return z
}

func main() {
	p := 101
	q := 113
	n := p * q

	n1 := 11200
	b := 3533
	_b := inverse(b, n1)

	// pubkey (n, b) privatekey (p, q, _b)

	plaintext := []byte("你好")
	fmt.Println("Source text ", string(plaintext))

	var cipher []int

	for _, ch := range plaintext {
		e := squareAndMultipy(int(ch), b, n)
		cipher = append(cipher, e)
	}

	fmt.Println("Encrypt text: ", cipher)

	var text []byte
	for _, v := range cipher {
		d := squareAndMultipy(v, _b, n)
		text = append(text, byte(d))
	}
	fmt.Println("Decrypt text: ", string(text))
}
