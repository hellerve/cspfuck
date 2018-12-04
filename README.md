# cspfuck

`cspfuck` is Brainfuck with channels. Every paragraph of the source code is an
actor that is effectively a Brainfuck program with added primitives for
receiving from and sending to other actors.

This is the JIT version.

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
- `>`: Advance the tape head or wrap around.
- `<`: Retreat the tape head or wrap around.
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
- `u`: Receive a value, write it into the cell currently under the tape head.

And that’s all that `cspfuck` is.

## Example

I’m a terrible Brainfuck programmer, so I only provide one proof-of-concept
example for now. It is contained in a directory named
`example/`—appropriately singular—, in which I provide a ping pong program.
It contains two actors, one printing ping, then waiting for the other, the
other waiting, and then printing pong, ten times.

## Implementation

Actors are implemented as pthreads. The virtual machine is a simple handrolled
JIT that offers 30,000 elements to each Brainfuck program. It will segfault if
you go past the low or high threshold. This version is about four times faster
than the bytecode VM provided in the [`master`](https://github.com/hellerve/cspfuck/tree/master) branch.

It’s only about 550 lines of C, so it should be reasonably consumable. The
code isn’t necessarily pretty, but it seems to work well. It is not incredibly
battle-tested, though.

**Disclaimer**: I know approximately as much about concurrent programming in C
as I know about writing production-grade Brainfuck. The system should be
expected to be brittle.

<hr/>

Have fun!
