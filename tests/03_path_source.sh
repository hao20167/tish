echo ===== path, addpath and source =====
mkdir -p tests/output/bin
ln -sf /bin/echo tests/output/bin/privateecho
path
addpath tests/output/bin
path
privateecho private-path-works
echo this-file-is-running-through-source
