# create account
createAccount() {
    contractAccount=$1
    createAccountScript="cleos wallet create -n ${contractAccount}.wallet --to-console"
    ret=`ssh sh-misc "${remoteDockerScrip} '${createAccountScript}'"`
    echo $ret
    #get priv key 
    privKey=${ret:0-54:53}
    return 0
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
echo "-----buildContract     函数开始执行-----"
# buildContract
echo "-----buildContract     函数结束执行-----"
# scpToMiscMerchine
echo "-----scpToMiscMerchine 函数结束执行-----"

##create account
otcContractName="${otcFileName}.${accountTail}"
createAccount $otcContractName
echo "-----createAccount 函数结束执行----- $privKey"

##deploy contract
# createContract ${otcFileName} ${otcContractName}

#otcConfName='otcconf'
#otcContractName="${otcConfName}.${accountTail}"