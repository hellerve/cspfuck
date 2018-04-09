# cspfuck

`cspfuck` is Brainfuck with channels. Every paragraph of the source code is an
actor that is effectively a Brainfuck program with added primitives for
receiving from and sending to other actors.

## Usage

You can build the program through its Makefile by typing `make`. This will leave
you with an executable called `cspfuck` in the `bin/` directory that you can
feed cspfuck files through the command line.

## The language

Standard Brainfuck has eight operations. You can find out about them on the web,
but for ease of use I will repeat them here. Brainfuck operates on a global
tape of—typically—30,000 elements, initially all set to 0.

- `+`: Increment the value currently under the tape head.
- `-`: Decrement the value currently under the tape head.
- `>`: Advance the tape head.
- `<`: Retreat the tape head.
- `.`: Print out the value currently under the tape head, interpreted as
       ASCII code.
- `,`: Read a character into the cell currently under the tape head,
       interpreted as a number by its ASCII code.
- `[`: Start of a loop. Loops will be executed unless the value currently under
       the tape head is zero.
- `]`: End of a loop. Will jump back to the matching `[`, with respect of
       nesting.

That’s all that Brainfuck is.

`cspfuck` adds three primitives to this set, namely:

- `^`: Send the value currently under the tape head to the actor above.
- `v`: Send the value currently under the tape head to the actor below.
- `u`: Receive a value, write it into the cell currently unter de tape head.

And that’s all `cspfuck` is.

<hr/>

Have fun!
