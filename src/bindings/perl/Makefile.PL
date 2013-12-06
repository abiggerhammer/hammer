use ExtUtils::MakeMaker;
use File::Basename;
use Config;

# Scons hack...
chdir(dirname($0));

WriteMakefile(
    NAME => "hammer",
    LIBS => ["-lhammer"],
    OBJECT => 'hammer_wrap.o',
    INC => '-I../..',
    CCFLAGS => "$Config{ccflags} -DSWIG -DHAMMER_INTERNAL__NO_STDARG_H -std=gnu99",
    );

