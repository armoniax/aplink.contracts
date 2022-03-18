updateContract(){
  contractName=$1
  contractFilePath='/opt/mgp/node_devnet/data/otccontract'
  updateContract="cleos set contract ${contractName} ${contractFilePath} ${otcFileName}.wasm ${otcFileName}.abi -p ${contractName}@active"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}
contractName=$1
otcFileName=$2
remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
