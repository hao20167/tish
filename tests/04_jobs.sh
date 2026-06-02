echo ===== job control =====
sleep 10 &
jobs
kill -STOP %+
jobs
bg %+
jobs
kill -CONT %+
kill -9 %+
sleep 1 &
fg %
echo foreground-finished
