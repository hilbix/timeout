# timeout

- Kill a program after running N seconds
- Terminate a pipe after N seconds
- Terminate a pipe after N seconds inactivity

Similar program:

- https://github.com/hilbix/checkrun checks for program output inactivity

## Usage

	git clone https://github.com/hilbix/timeout.git
	cd timeout
	make
	sudo make install

Then:

	timeout [-v] [-signal[@seconds]...] seconds [-|program [args...]]

## Example

	timeout -9 5 command args		# kill -9 command after 5 seconds
	producer | timeout 5 | consumer		# terminate pipe after 5 seconds
	producer | timeout 5 - | consumer	# terminate pipe after 5 second inactivity

## More help

See [DESCRIPTION](DESCRIPTION)

