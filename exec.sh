git submodule deinit -f googletests/googletest
git rm -f googletests/googletest
rm -rf .git/modules/googletests
rm -rf googletests/googletest
git add .gitmodules
git commit -m "Clean up broken googletest submodule"
git submodule add https://github.com/google/googletest.git googletests/googletest
git submodule update --init --recursive