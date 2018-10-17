if [ -n "$(pgrep AServer.out)" ]; then
	echo Process $(pgrep AServer.out) killed
	kill $(pgrep AServer.out)
fi
