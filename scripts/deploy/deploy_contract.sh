# create key
createKey() {
    unlockScript='cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog'
    ssh sh-misc "${remoteDockerScrip} '${unlockScript}'"
    createAccountScript='cleos create key --to-console'
    ret=`ssh sh-misc "${remoteDockerScrip} '${createAccountScript}'"`
    echo "create key: $ret"
    privKey=${ret:13:51}
    pubKey=`echo $ret | sed -n '1p'`
    pubKey=${pubKey:0-54:54}
}

#newAccountAndActive
newAccountAndActive(){
  STAKE_NET='1.0000 MGP'
  STAKE_CPU='1.0000 MGP'
  newAccountAndActiveScript="cleos system newaccount eosio ${accountTail} ${pubKey} ${pubKey} --stake-net \"${STAKE_NET}\" --stake-cpu \"${STAKE_CPU}\" --buy-ram-kbytes 1100"
  ret=`ssh sh-misc "${remoteDockerScrip} '${newAccountAndActiveScript}' "`
  echo "newAccountAndActive output: $ret"
}

# create contract
# param1: file name
createContract() {
    otcFileName=$1
    otcContractName=$2
 
    unlockScript='cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog'
    ssh sh-misc "${remoteDockerScrip} '${unlockScript}'"

    contractFilePath='/opt/mgp/node_devnet/data/otccontract'
    otcFileName='otcbook'
    setContractScript="cleos set contract ${otcContractName} ${contractFilePath} ${otcFileName}.wasm ${otcFileName}.abi -p ${otcContractName}@active"
    ssh sh-misc "${remoteDockerScrip} '${setContractScript}'"
}

#build
buildContract(){
    DOCKER_ID=amax-dev-sean
    buildScript='cd /opt/data/build && rm -rf ./* && cmake .. && make -j8'
    docker exec -i $DOCKER_ID /bin/bash -c "${buildScript}"
}

#scp
scpToMiscMerchine(){
    remoteContractPath=/mnt/data/mgp-test/eosio-docker/node_devnet/data/otccontract
    scp build/contracts/otcbook/otcbook* sh-misc:${remoteContractPath}
    scp build/contracts/otcconf/otcconf* sh-misc:${remoteContractPath}
}

accountTail=$1
remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
otcFileName='otcbook'

##create account
echo "--------------------------        createKey 函数开始执行            --------------------------"
otcContractName="${otcFileName}.${accountTail}"
createKey $otcContractName

echo "--------------------------        createKey 函数结束执行            --------------------------"

##newAccountAndActive
echo "--------------------------    newAccountAndActive  函数开始执行     --------------------------"
#newAccountAndActive
echo "--------------------------    newAccountAndActive  函数结束执行     --------------------------"


##deploy contract
# createContract ${otcFileName} ${otcContractName}

#otcConfName='otcconf'
#otcContractName="${otcConfName}.${accountTail}"