
OTPROOT=$(wildcard /opt/erlang/lib/erl_interface-*)
OTPROOT_LOCAL=$(wildcard /usr/local/Cellar/erlang/20.0/lib/erlang/lib/erl_interface-*)

all:	bin/cnodeserver bin/cnodeclient 
local:	local/cnodeserver

bin/Elixir.Complex.beam: src/complex.ex
	elixirc -o bin $<

local/%:	src/%.c
	mkdir -p bin
	gcc -o -g $@ -I$(OTPROOT_LOCAL)/include -L//usr/lib/x86_64-linux-gnu/ -L$(OTPROOT_LOCAL)/lib src/lamewrapper.c $< -lerl_interface -I./include -lei -lpthread -lmp3lame

bin/%:	src/%.c
	mkdir -p bin
	gcc -ggdb -o  $@ -I$(OTPROOT)/include -L//usr/lib/x86_64-linux-gnu/ -L$(OTPROOT)/lib src/lamewrapper.c $< -lerl_interface -I./include -lei -lpthread -lnsl -lmp3lame

clean:
	rm -rf bin


start_server:
	epmd -daemon
	bin/cnodeserver 3456

start_client:
	echo "run 'complex3:foo(4).' via erlang shell"
	erl -sname e1 -setcookie secretcookie -pa bin

start_elixir:
	echo "run ':complex3.foo(4)' via elixir shell"
	iex --sname e1 --cookie secretcookie -pa bin
