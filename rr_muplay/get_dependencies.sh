# Check to see if this directory exists already
obj_dir=$(pwd)
mkdir -p dependencies
cd dependencies
# download and build libunwind
file="./libunwind-1.3-rc1/src/.libs/libunwind.a"
if [ ! -e "$file" ]
then
    echo "$0: File '${file}' not found."
    wget http://download.savannah.nongnu.org/releases/libunwind/libunwind-1.3-rc1.tar.gz
    tar -zxvf libunwind-1.3-rc1.tar.gz
    rm libunwind-1.3-rc1.tar.gz
    cd libunwind-1.3-rc1
    ./configure --prefix=$(pwd) --disable-shared
    make
    make install
    mv lib/libunwind-generic.a src/.libs
    cd ..
else
    echo "libunwind exists."
fi
# download and build libdwarf
file="./libdwarf-obj/lib/libdwarf.so"
if [ ! -e "$file" ]
then
    git clone https://github.com/tomhughes/libdwarf.git
    cd libdwarf
    mkdir ../libdwarf-obj
    ./configure --enable-shared --prefix=$(pwd)/../libdwarf-obj
    make
    make install
    cd ..
else
    echo "libdwarf exists."
fi
#download and build libelf
file="./libelf-0.8.13/lib/libelf.a"
if [ ! -e "$file" ]
then
    # This link seems to be broken
    # wget http://www.mr511.de/software/libelf-0.8.13.tar.gz
    wget https://fossies.org/linux/misc/old/libelf-0.8.13.tar.gz
    tar -zxvf libelf-0.8.13.tar.gz
    rm libelf-0.8.13.tar.gz
    cd libelf-0.8.13
    ./configure
    make
    cd ..
else
    echo "libelf exists."
fi
#download and build libz
file="./zlib-1.2.11/libz.a"
if [ ! -e "$file" ]
then
    wget https://zlib.net/zlib-1.2.11.tar.gz
    tar -zxvf zlib-1.2.11.tar.gz
    rm zlib-1.2.11.tar.gz
    cd zlib-1.2.11
    ./configure
    make
    cd ..
else
    echo "libz exists."
fi

#Not using this. Instead will use same version as Egalito
#download and build capstone
#file="./capstone/libcapstone.a"
#if [ ! -e "$file" ]
#then
#    git clone https://github.com/aquynh/capstone.git
#    cd capstone
#    ./make.sh
#    cd ..
#else
 #   echo "capstone exists."
#fi
#cd ..

#download and build egalito
# -- NOTE THIS DOESN'T INSTALL DEPENDENCIES
file="${obj_dir}/dependencies/egalito/src/libegalito.so"
if [ ! -e "$file" ]
then
	cd ${obj_dir}/dependencies
	git clone git@github.com:columbia/egalito.git --recursive
	cd egalito
	git checkout merging
	git submodule update --init --recursive
	make -j8
else
	echo "egalito exists"
fi
echo "All dependencies are installed."
cd $obj_dir
#./configure
#make
#make install prefix=$currentdir/dependencies
