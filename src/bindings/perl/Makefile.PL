use ExtUtils::MakeMaker;
use File::Basename;
use Config;

# Scons hack...
chdir(dirname($0));

WriteMakefile(
    NAME => "hammer",
    LIBS => ["-L../../ -lhammer"],
    OBJECT => 'hammer_wrap.o',
    INC => '-I../..',
    CCFLAGS => "$Config{ccflags} -DSWIG -DHAS_BOOL -std=gnu99",
    );

