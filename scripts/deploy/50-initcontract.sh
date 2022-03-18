unlock() {
  unlock="cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog"
   ssh sh-misc "${remoteDockerScrip} '${unlock}'"
}

initContract() {
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

unlock
initContract 
setPermission