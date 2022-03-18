initData() {
  contractFilePath='/opt/mgp/node_devnet/data/otccontract'
  ssh sh-misc "${remoteDockerScrip} 'sh ${contractFilePath}/60-remote-initdata.sh ${otcAccountName}'"
}

remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
otcAccountName=${1}.o

initData