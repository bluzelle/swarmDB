#!/usr/bin/env bash
rm -rf ../../bluzelled/assets/web/
mkdir ../../bluzelled/assets/web/
cp ../dist/index.html ../../bluzelled/assets/web/

mkdir ../../bluzelled/assets/web/generated
mkdir ../../bluzelled/assets/web/generated/js/

cp ../dist/generated/js/index.js ../../bluzelled/assets/web/generated/js/