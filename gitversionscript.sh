EIWOMISARC_GITREV=$(git rev-parse HEAD)
echo "#define GITREV \""$EIWOMISARC_GITREV"\"" > git_rev.h
unset EIWOMISARC_GITREV