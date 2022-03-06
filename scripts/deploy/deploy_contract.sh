
#build
DOCKER_ID=amax-dev-sean
buildScript='cd /opt/data/build && rm -rf ./* && cmake .. && make -j8'
docker exec -it $DOCKER_ID /bin/bash -c ${buildScript}

#scp
remoteContractPath=/mnt/data/mgp-test/eosio-docker/node_devnet/data/otccontract
scp build/contracts/otcbook/otcbook* sh-misc:${remoteContractPath}
scp build/contracts/otcconf/otcconf* sh-misc:${remoteContractPath}

#deploy
remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c '
unlockScript='cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog'
ssh sh-misc "docker exec -i mgp-devnet /bin/bash -c '${unlockScript}'"


#sh-misc target
#/mnt/data/mgp-test/eosio-docker/node_devnet/data/otccontract