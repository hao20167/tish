echo ===== pipeline and redirection =====
mkdir -p tests/output
echo alpha > tests/output/demo.txt
echo beta >> tests/output/demo.txt
cat < tests/output/demo.txt
cat < tests/output/demo.txt | grep beta
false || echo recovered-with-or
true && echo continued-with-and
echo first ; echo second
