# 8042 Scancodes

## Single-byte

* high bit 0: keydown
* high bit 1: keyup

```
01	Esc
02	1
03	2
04	3
05	4
06	5
07	6
08	7
09	8
0a	9
0b	0
0c	-
0d	=
0e	Backspace
0f	Tab
10	Q
11	W
12	E
13	R
14	T
15	Y
16	U
17	I
18	O
19	P
1a	[
1b	]
1c	Enter
1d	LCtrl
1e	A
1f	S
20	D
21	F
22	G
23	H
24	J
25	K
26	L
27	;
28	'
29	`
2a	LShift
2b	\
2c	Z
2d	X
2e	C
2f	V
30	B
31	N
32	M
33	,
34	.
35	/
36	RShift
37	KP-*
38	LAlt
39	space
3a	CapsLock
3b	F1
3c	F2
3d	F3
3e	F4
3f	F5
40	F6
41	F7
42	F8
43	F9
44	F10
45	NumLock
46	ScrollLock
47	KP-7 / Home
48	KP-8 / Up
49	KP-9
4a	KP--
4b	KP-4 / Left
4c	KP-5
4d	KP-6 / Right
4e	KP-+
4f	KP-1 / End
50	KP-2
51	KP-3 / PgDn
52	KP-0 / Ins
53	KP-. / Del
54	Alt+SysRq
57	F11
58	F12
```

## Extended

```
e0 1c	KP-Enter
e0 1d	RCtrl
e0 21	Calculator
e0 32	Web / Home
e0 35	KP-/
e0 37	PrtScr
e0 38	RAlt
e0 46	Ctrl+Break
e0 47	Home
e0 48	Up
e0 49	PgUp
e0 4b	Left
e0 4d	Right
e0 4f	End
e0 50	Down
e0 51	PgDn
e0 52	Insert
e0 53	Delete
e0 5b	LWin (USB: LGUI)
e0 5c	RWin (USB: RGUI)
e0 5d	Menu
e0 5e	Power
e0 5f	Sleep
e0 63	Wake
e0 65	Search
e0 66	Favorites
e0 68	Stop
e0 69	Forward
e0 6a	Back
e0 6b	My Computer
e0 6c	Mail
```

## Wakeup/ACK

Upon waking from sleep, I've observed the following sequence of bytes flow through the filter.
I believe it's a sequence of Ack (0xfa) and Keyboard ID (0xab 0x41) responses.

```
fa ab 41 fa fa fa fa fa fa fa fa fa fa
```

I need to prevent the filter from tripping over these bytes, as they don't come in pairs like real
keydown/keyup events, nor are they relevant for debouncing. Since I also don't want to mess with
extended codes (e.g. media buttons), I'll put a few guards at the top of the filter:

  * `data == e0` → next byte carries data for an extended code
  * `prev-data == e0` → this byte carries data for an extended code
  * `data & 7f == 2b` → this byte is either a \ key or the first byte of a keyboard ID sequence
  * `(data & 7f) < 01 || 3a < (data & 7f)` → this byte is outside the range of keys I care to debounce

To understand the third guard, note that any attempt to debounce \ would be unable to distinguish a
keyup (`ab`) from the beginning of a keyboard ID sequence (`ab 41`). Since \ is an unlikely source
of bounce, I'm simply ignoring it.

The last guard (range check) lets me omit an explicit guard for Acks (`fa`) or the second byte in a
keyboard ID sequence (`41`), as both fall outside the specified range.

# References

https://www.win.tue.nl/~aeb/linux/kbd/table.h
http://kbd-project.org/docs/scancodes/scancodes-10.html
http://www.scs.stanford.edu/18wi-cs140/pintos/specs/kbd/scancodes-9.html
