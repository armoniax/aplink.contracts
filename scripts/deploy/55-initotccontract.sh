unlock() {
  unlock="cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog"
   ssh sh-misc "${remoteDockerScrip} '${unlock}'"
}

initDeotcContract() {
  updateContract="cleos push action ${otcAccountName} init \"[ '${confAccountName}']\" -p  ${otcAccountName}@active"
  echo "-------initContract--------------"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}

remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
otcAccountName=${1}.o
confAccountName=${1}.h

unlock
initDeotcContract