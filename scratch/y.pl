:- initialization((main2)).

clean_text_input :-
	clean_file('test_input.text', _).

clean_file(File, Path) :-
	absolute_file_name(File, Path),
	% the file can be associated with more than one stream
	forall(
		stream_property(Stream, file_name(Path)),
		close(Stream)
	),
	% the file may exist only if some unit test failed
	(	exists_file(Path) ->
		delete_file(Path)
	;	true
	).

clean_file(Alias, File, Path) :-
	% the alias may be associated with a stream opened for a different file
	(	stream_property(Stream, alias(Alias)) ->
		close(Stream)
	;	true
	),
	clean_file(File, Path).

write_text_contents(Stream, Contents) :-
	(	atom(Contents) ->
		write(Stream, Contents)
	;	write_text_contents_list(Contents, Stream)
	).

write_text_contents_list([], _).
write_text_contents_list([Atom| Atoms], Stream) :-
	write(Stream, Atom),
	write_text_contents_list(Atoms, Stream).

set_text_input(Alias, Contents, Options) :-
	clean_file(Alias, 'test_input.text', Path),
	open(Path, write, WriteStream, [type(text)]),
	write_text_contents(WriteStream, Contents),
	close(WriteStream),
	open(Path, read, _, [type(text),alias(Alias)| Options]).

set_text_input(Alias, Contents) :-
	set_text_input(Alias, Contents, []).

set_text_input(Contents) :-
	clean_file('test_input.text', Path),
	open(Path, write, WriteStream, [type(text)]),
	write_text_contents(WriteStream, Contents),
	close(WriteStream),
	open(Path, read, ReadStream, [type(text)]),
	set_input(ReadStream).

get_chars(Stream, Chars, Countdown) :-
	get_char(Stream, Char),
	(	Char == end_of_file ->
		Chars = [],
		close(Stream)
	;	Countdown =< 0 ->
		Chars = []
	;	Chars = [Char| Rest],
		NextCountdown is Countdown - 1,
		get_chars(Stream, Rest, NextCountdown)
	).

get_text_contents(Stream, Expected, Contents) :-
	atom_length(Expected, Length),
	Limit is Length + 1,
	get_chars(Stream, Chars, Limit),
	atom_chars(Contents, Chars).

check_text_input(Alias, Expected) :-
	get_text_contents(Alias, Expected, Contents),
	clean_text_input,
	Expected == Contents.

check_text_input(Expected) :-
	current_input(Stream),
	get_text_contents(Stream, Expected, Contents),
	clean_text_input,
	Expected == Contents.

main1 :-
	set_text_input('qwerty'),
	get_char(Char),
	Char == 'q',
	check_text_input('werty'),
	write(ok), nl,
	halt.

main2 :-
	set_text_input(st_i, 'qwerty'),
	get_char(st_i, Char),
	Char == 'q',
	check_text_input(st_i, 'werty'),
	write(ok), nl,
	halt.

