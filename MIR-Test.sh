# on icub22 unplug icub22 from the cluster and change the namespace with
# yarp namespace /root
# then uncomment the following line
unset YARP_ROBOT_NAME

if [ $# -lt 3 ]; then
    echo "Usage: $0 <abspath-to-build> <abspath-to-code> <abspath-to-test>"
    exit 4
fi

build_dir=$1
code_dir=$2
test_dir=$3
cur_dir=$(pwd)

cd $build_dir
if [ -d build-code ]; then   #returns true if the directory build_code exists
    rm build-code -rf
fi

mkdir build-code && cd build-code
cmake $code_dir

# $? is the most recent foreground pipeline exit status, -ne stands for not equal
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 2
fi
make install
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 2
fi

cd ../

if [ -d build-test ]; then 
    rm build-test -rf
fi

mkdir build-test && cd build-test
cmake $test_dir
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 3
fi
make
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 3
fi
cd ../../suits

# to let yarpmanager access the fixture
export YARP_DATA_DIRS=$cur_dir/suits:${YARP_DATA_DIRS}
#echo "DBG: $YARP_DATA_DIRS"

# to make the test library retrievable
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$cur_dir/src/test/build

testrunner --verbose --suit $cur_dir/suits/MIR-TestSuit.xml # > result.txt

cd $cur_dir

