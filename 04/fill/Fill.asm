// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Fill.asm

// Runs an infinite loop that listens to the keyboard input.
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel;
// the screen should remain fully black as long as the key is pressed.
// When no key is pressed, the program clears the screen, i.e. writes
// "white" in every pixel;
// the screen should remain fully clear as long as no key is pressed.

// Put your code here.
@8192
D = A
@SCREEN
D = D+A
@MAXADDRESS
M = D

(key)
@SCREEN
D = A
@address
M = D

@KBD
D = M

@WHITE
D; JEQ
@BLACK
0; JMP

(WHITE)
@color
M = 0
@loop
0; JMP

(BLACK)
@color
M = -1
@loop
0; JMP

(loop)
@color
D = M

@address
A = M
M = D

D = A+1
@address
M = D

@MAXADDRESS
D = M-D

@loop
D; JNE

@key
0; JMP

(end)