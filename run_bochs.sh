echo c > comm_test
nice -n 7 bochsdbg -qf support/bochsrc.txt -rc comm_test
rm -f comm_test