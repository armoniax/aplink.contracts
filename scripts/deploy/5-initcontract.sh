updateContract(){
  contractFilePath='/opt/mgp/node_devnet/data/otccontract'
  updateContract="cleos set contract ${accountName} ${contractFilePath} ${otcFileName}.wasm ${otcFileName}.abi -p ${accountName}@active"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}

accountName=$1
otcFileName=$2
remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'

updateContract 