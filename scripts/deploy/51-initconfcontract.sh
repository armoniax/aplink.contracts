
initConfContract() {
  updateContract="cleos push action ${confAccountName} init \"[]\" -p  ${confAccountName}@active"
  echo "-------initContract--------------"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}

setPermission() {
  updateContract="cleos set account permission ${otcAccountName} active --add-code"
  echo "-------setPermission--------------"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}

remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
otcAccountName=${1}.o
confAccountName=${1}.h

sh ./scripts/deploy/01-unlock.sh
initConfContract 
setPermission