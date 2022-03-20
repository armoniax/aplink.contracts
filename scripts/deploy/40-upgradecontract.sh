updateContract(){
  contractFilePath='/opt/mgp/node_devnet/data/otccontract'
  updateContract="cleos set contract ${accountName} ${contractFilePath} ${otcFileName}.wasm ${otcFileName}.abi -p ${accountName}@active"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}

remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
accountName=${1}.o
otcFileName='otcbook'
updateContract 

accountName=${1}.h
otcFileName='otcconf'
# updateContract 