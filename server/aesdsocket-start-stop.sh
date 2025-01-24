#!/bin/sh
# $1 is the name of the first argument
case "$1" in
    start)
        echo "Starting aesdsocket server daemon"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket server daemon"
        start-stop-daemon -K -n aesdsocket -s TERM
        ;;
    restart)
        $0 stop
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit 0
