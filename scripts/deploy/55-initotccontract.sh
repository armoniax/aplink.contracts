
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