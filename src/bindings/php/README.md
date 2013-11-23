Installing
==========
Requirements:
* SWIG 2.0
* A properly configured [phpenv](https://github.com/CHH/phpenv)

If you want to run the tests, you will also need to install PHPUnit. Do this with pyrus and save yourself some hell. 

    pyrus channel-discover pear.phpunit.de
    pyrus channel-discover pear.symfony.com
    pyrus channel-discover pear.symfony-project.com
    pyrus install --optionaldeps phpunit/PHPUnit
