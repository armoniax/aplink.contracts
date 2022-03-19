initData() {

  importPrivkeyShell="cleos wallet import --private-key ${privKey}"
  echo "-------import priv key--------------"
  ssh sh-misc "${remoteDockerScrip} '${importPrivkeyShell}'"

  setMerchantShell="cleos push action ${otcAccountName} setmerchant  \"[ '${merchant}' , '${merchant}' , '${merchant}' , 'xxxama.com', 'dddddddd']\" -p ${merchant}@active"
  ssh sh-misc "${remoteDockerScrip} '${setMerchantShell}'"
}


sh ./scripts/deploy/01-unlock.sh
otcAccountName=${1}.o
remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'