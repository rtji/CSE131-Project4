tatic vars
ROOT="public_samples/"
VERSION="1.0.0"

#Counters
numPass=0
numTest=0

#Command line args
showPass=true
showFail=true
verbose=true
onlyTestCase=""

color() {
  c="$1"
  shift
  echo -e "\e[${c}m${@}\e[0m"
}

while true; do
    case "$1" in
        "--hidepass") showPass=false;  shift;;
        "--hidefail") showFail=false;  shift;;
        "--short")    verbose=false;   shift;;
        "--version")  echo "$VERSION"; exit;;
        "--onlytest") shift;           onlyTestCase=$1; shift;;
        *)            break;;
    esac
done

#BUILD GLC
echo
color 35 "Building ./glc..."
echo
make
make_status="$?"

if [ "$make_status" != 0 ]; then
    color 31 "Build failed."
    echo $make_out
    exit 1 
fi

[ -x glc ] || { echo "Error: glc not executable"; exit 1; }
[ -x p4exe ] || { echo "Error: p4exe not found or executable"; exit 1; }

for file in $ROOT*.glsl; do
    fileNoExt="${file%.glsl}"

    fileName="${fileNoExt:${#ROOT}}"
    if [ "$onlyTestCase" != "" ] && [ "$onlyTestCase" != "$fileName" ]; then
        continue
    fi

    let numTest+=1

    goldenSegfault=false
    userSegfault=false

    (eval ./p4exe < $file > deleteMeGolden 2> /dev/null) 2> /dev/null || goldenSegfault=true
	(eval "./glc < $file > deleteMeUser 2> deleteMeErr") 2> /dev/null || userSegfault=true
   
    #Check for segfault
    if [ "$goldenSegfault" == true ] || [ "$userSegfault" == true ]; then
        if [ "$showFail" == true ]; then
            color 31 FAILED $fileName
            if [ "$verbose" == true ]; then
                cat deleteMeErr
                if [ "$goldenSegfault" == true ]; then
                    echo "segfault in p4exe"
                fi
                if [ "$userSegfault" == true ]; then
                    echo "segfault in glc"
                fi
            fi
        fi
        continue
    fi

    mv deleteMeGolden $fileNoExt.bc
    (eval ./gli $fileNoExt.bc > deleteMeGolden 2> deleteMeGLIErrGolden) 2> /dev/null || goldenSegfault=true

    mv deleteMeUser $fileNoExt.bc
    (eval ./gli $fileNoExt.bc > deleteMeUser 2> deleteMeGLIErrUser) 2> /dev/null || userSegfault=true
    rm $fileNoExt.bc

    failed=false
    if [ -s deleteMeGLIErrGolden ]; then
        echo >> deleteMeErr
        echo "gli (Golden) stderr:" >> deleteMeErr 2> /dev/null
        echo "************************************************" >> deleteMeErr
        cat deleteMeGLIErrGolden >> deleteMeErr 2> /dev/null
        echo "************************************************" >> deleteMeErr
    fi
    if [ -s deleteMeGLIErrUser ]; then
        echo >> deleteMeErr
        echo "gli (User) stderr:" >> deleteMeErr 2> /dev/null
        echo "************************************************" >> deleteMeErr
        cat deleteMeGLIErrUser >> deleteMeErr 2> /dev/null
        echo "************************************************" >> deleteMeErr
    fi

    cmp --silent deleteMeUser deleteMeGolden || failed=true
    if [ "$failed" == true ] || [ "$goldenSegfault" == true ] || [ "$userSegfault" = true ]; then
        if [ "$showFail" == true ]; then
            color 31 FAILED $fileName
            if [ "$verbose" == true ]; then
                cat deleteMeErr
		if [ "$goldenSegfault" != true ] && [ "$userSegfault" != true ]; then
                	diff -wu --label "glc" deleteMeUser --label "p4exe" deleteMeGolden
		fi
		if [ "$goldenSegfault" == true ]; then
                    echo "segfault in p4exe"
                fi
                if [ "$userSegfault" == true ]; then
                    echo "segfault in glc"
                fi
            fi
        fi
    else
        let numPass+=1
        if [ "$showPass" == true ]; then
            color 32 PASSED $fileName
        fi
    fi
done
echo
color 35 "TOTAL: $numPass/$numTest"
rm deleteMe*
