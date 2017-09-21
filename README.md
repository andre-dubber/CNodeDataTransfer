CNodeDataTransfer
====================

Based on https://github.com/arnehilmann/erlang-cnode-example

Compiles works on linux, fails on OSX

## Usage

Build

    $ cd cnode
    $ make

Start CNode as server

    $ ./bin/cnodeserver 3456

In another terminal, start iex

    $ cd elixir
    $ iex  --sname e1 --cookie secretcookie -r encoder.ex

Send request with Elixir module

    iex> Mp3Stream.Encoder.encode2("/tmp/1.wav")
