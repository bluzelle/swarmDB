#!/usr/bin/env sh
ports=()
while read p; do
    IFS="="
    read -a parts <<< "${p}"
    IFS=":"
    read -a parts <<< "${parts[1]}"
    ports+=(${parts[1]})
done < peers

for port in "${ports[@]}"
do
   echo "./the_db --address ${ROPSTEN_ADDR} --port ${port}"
done


