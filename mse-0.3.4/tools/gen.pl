# Generate a header and source file

$file  = $ARGV[0];
$macro = uc $file;
$macro =~ s@/@_@g;

# read templates

open F, "../src/code_template.hpp";
$hpp = join('',<F>);
close F;

open F, "../src/code_template.cpp";
$cpp = join('',<F>);
close F;

# insert stuff

$hpp =~ s/_\n/_$macro\n/g;

$cpp =~ s@<util/prec.hpp>@<$file.hpp>@g;

# write files

open F, "> ../src/$file.hpp";
print F $hpp;
close F;

open F, "> ../src/$file.cpp";
print F $cpp;
close F;