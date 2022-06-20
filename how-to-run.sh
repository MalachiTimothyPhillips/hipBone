module load openblas
export OPENBLAS_DIR=$OLCF_OPENBLAS_ROOT/lib # <- for where to find BLAS
make gpu-aware-mpi=true -j 8
