#!/bin/bash

if [ "$#" -lt 3 ]
then
  echo "Usage:  makeapp.sh <dest_dir> <scantailor_source_dir> <build_dir>"
  echo " "
  echo "dest_dir is where the output files are created."
  echo "scantailor_source_dir is where the scantailor source files are."
  echo "build_dir is the working directory where dependency libs are built."
  echo ""
  exit 0
fi

DESTDIR=$1
SRCDIR=$2
BUILDDIR=$3

# I’ve hardcoded path to macdeployqt [truf]
MACDEPLOYQT=$DESTDIR"/../Qt/5.8/clang_64/bin/"

export APP=$DESTDIR/ScanTailorUniversal.app
export APPC=$APP/Contents
export APPM=$APPC/MacOS
export APPR=$APPC/Resources
export APPF=$APPC/Frameworks

rm -rf $APP
mkdir -p $APPC
mkdir -p $APPM
mkdir -p $APPR
mkdir -p $APPF

cp $SRCDIR/src/packaging/osx/ScanTailorUniversal.icns $APPR/ScanTailorUniversal.icns
cp $SRCDIR/src/app/scantailor-universal.app/Contents/MacOS/scantailor-universal $APPM/ScanTailorUniversal


mkdir $APPR/translations/
cp $SRCDIR/src/scantailor-universal_*.qm $APPR/translations/
cp $SRCDIR/src/translations/qtbase_*.qm $APPR/translations/

mkdir $APPR/stylesheets/
cp $SRCDIR/src/stylesheets/*.qss $APPR/stylesheets/
cp $SRCDIR/src/stylesheets/BreezeStyleSheets/*.qss $APPR/stylesheets/
cp -R $SRCDIR/src/stylesheets/BreezeStyleSheets/Breeze $APPR/stylesheets/
cp -R $SRCDIR/src/stylesheets/BreezeStyleSheets/BreezeDark $APPR/stylesheets/

stver=`cat $SRCDIR/version.h | grep 'VERSION "' | cut -d ' ' -f 3 | tr -d '"'`
cat $SRCDIR/src/packaging/osx/Info.plist.in | sed "s/@VERSION@/$stver/" >  $APPC/Info.plist

otool -L $APPM/ScanTailorUniversal | tail -n +2 | tr -d '\t' | cut -f 1 -d ' ' | while read line; do
  case $line in
    $BUILDDIR/*)
      ourlib=`basename $line`
      cp $line $APPF >/dev/null 2>&1
      install_name_tool -change $line @executable_path/../Frameworks/$ourlib $APPM/ScanTailorUniversal
      install_name_tool -id @executable_path/../Frameworks/$ourlib $APPF/$ourlib
      ;;
    esac
done

rm -rf ScanTailorUniversal.dmg $DESTDIR/ScanTailorUniversal-$stver.dmg
cd $DESTDIR

$MACDEPLOYQT/macdeployqt $DESTDIR/ScanTailorUniversal.app -dmg >/dev/null 2>&1
mv ScanTailorUniversal.dmg $DESTDIR/ScanTailorUniversal-$stver.dmg

