# on icub22 unplug icub22 from the cluster and change the namespace with
# yarp namespace /root
# then uncomment the following line
#export YARP_ROBOT_NAME=icubSim 

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
cmake -DCMAKE_BUILD_TYPE=Release $code_dir

# $? is the most recent foreground pipeline exit status, -ne stands for not equal
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 2
fi
make DESTDIR=$cur_dir install
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 2
fi
cd ../

if [ -d build-test ]; then 
    rm build-test -rf
fi

mkdir build-test && cd build-test
cmake -DCMAKE_BUILD_TYPE=Release $test_dir
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 3
fi
make DESTDIR=$cur_dir install
if [ $? -ne 0 ]; then
   cd $cur_dir
   exit 3
fi
cd ../../suits

# to let yarpmanager access the fixture
export YARP_DATA_DIRS=${YARP_DATA_DIRS}:$cur_dir/suits

# to make the test library retrievable
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$cur_dir/usr/local/lib

# to make the executable retrievable
export PATH=$PATH:$cur_dir/usr/local/bin

testrunner --verbose --suit $cur_dir/suits/MIR-TestSuit.xml # > result.txt

#cd $cur_dir/build-code
#make uninstall && cd ../

# color codes
red='\033[1;31m'
green='\033[1;32m'
nc='\033[0m'

npassed=0
if [ -f result.txt ]; then
    cat result.txt
    npassed=$(grep -i "Number of passed test cases" result.txt | sed 's/[^0-9]*//g')
else
    echo -e "${red}Unable to get test result${nc}\n"    
fi

cd $cur_dir

