Building
========
Requirements:
* SWIG 2.0
* A properly configured [phpenv](https://github.com/CHH/phpenv)

SCons finds your PHP include path from `php-config`, so if you don't have that working, you're going to have a bad time.

If you want to run the tests, you will also need to install PHPUnit. Do this with pyrus and save yourself some hell. 

    pyrus channel-discover pear.phpunit.de
    pyrus channel-discover pear.symfony.com
    pyrus channel-discover pear.symfony-project.com
    pyrus install --optionaldeps phpunit/PHPUnit

Installing
==========
We're not building a proper package yet, but you can copy `build/$VARIANT/src/bindings/php/hammer.so` to your PHP extension directory (`scons test` will do this for you if you're using phpenv; for a system-wide php you'll probably have to use sudo) and add "extension=hammer.so" to your php.ini. There is a "hammer.ini" in src/bindings/php for your convenience; you can put it in the `conf.d` directory where PHP expects to find its configuration. `scons test` will do this for you too. You'll also need to point your include_path to the location of hammer.php, which will be `build/$VARIANT/src/bindings/php/hammer.php` until you put it somewhere else.