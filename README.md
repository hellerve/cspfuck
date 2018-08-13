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
- `u`: Receive a value, write it into the cell currently under the tape head.

And that’s all that `cspfuck` is.

## Example

I’m a terrible Brainfuck programmer, so I only provide one proof-of-concept
example for now. It is contained in a directory named
`example/`—appropriately singular—, in which I provide a hello world program.
It is three actors, printing hello world concurrently. At the end of the
program, the first actor goes back to the cell containing `d`, and sends it
to the actor below, which will receive it into a new cell, print it, and print
a newline to make it pretty.

## Implementation

Actors are implemented as pthreads. The virtual machine is a simple bytecode
VM that offers 30,000 elements to each Brainfuck program (it will actually
overflow if you go past that, oops).

It should be reasonably performant, but who cares? I hope noone’s going to run
their MapReduce jobs on it. There are some low-hanging fruits for optimization,
like making the VM loop use direct threaded code, but I chose not to for now.
Feel free to hack on it you want to! I’m happy to help you get started.

The VM does seem to execute [ridiculous programs](http://www.clifford.at/bfcpu/hanoi.html)
in standard Brainfuck pretty efficiently, which makes me unreasonably happy.

It’s only about 300 lines of C, so it should be reasonably consumable. The
code isn’t necessarily pretty, but it seems to work well. It is not incredibly
battle-tested, though.

If you want to know more, read [my blog post](http://blog.veitheller.de/Brainfuck_and_Actors.html)!

**Disclaimer**: I know approximately as much about concurrent programming in C as
I know about writing production-grade Brainfuck. The system should be expected
to be brittle.

<hr/>

Have fun!
