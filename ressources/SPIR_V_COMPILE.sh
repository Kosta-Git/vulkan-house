#!/usr/bin/env zsh

if [ -z "$1" ]
  then
    echo "Working in: " `pwd`
  else
    cd $1/ressources
    echo "Changed dir to: " `pwd`
fi

# shellcheck disable=SC2039
SHADERS=( 'triangle.frag' 'triangle.vert' )
COUNTER=0
SUCCESS=0

for shader_file in "${SHADERS[@]}"
do
  # shellcheck disable=SC2046
  echo Starting compilation for "$shader_file" "(" `expr $COUNTER + 1`/"${#SHADERS[@]}" ")"
  if glslangValidator -V -o "$shader_file".spv "$shader_file" | grep -q "$shader_file"; then
    echo Compiled "$shader_file" successfully!
    SUCCESS=`expr $SUCCESS + 1`
  fi
  echo -------------------------------------------------------------------------
  echo
  COUNTER=`expr $COUNTER + 1`
done

if [ "$SUCCESS" = "${#SHADERS[@]}" ]; then
  echo All shaders compiled successfully!

  else
    # shellcheck disable=SC2046
    echo `expr $COUNTER - $SUCCESS` shader failed to compile...
fi
