cl='bash /root/eosio-wallet/cleos.sh'

function setcode {
    $cl set contract $con ./build/contracts/mgp.bpvoting/
    $cl set account permission $con active '{"threshold": 1,"keys": [{"key": "'$pubkey'","weight": 1}],"accounts": [{"permission":{"actor":"'$con'","permission":"eosio.code"},"weight":1}]}' owner -p $con
}

function init {
  echo "0. conf & init"
  echo ""
  $cl push action $con config '[20,30,50,21,60,600,1,"100.0000 MGP","200.0000 MGP","10.0000 MGP"]' -p $con
  $cl push action $con init '[]' -p $con
}

function reward {
  echo "1. send rewards (300)"
  echo ""
  $cl transfer eosio $con "300.0000 MGP"
}

function list {
  echo "2. list masteraychen, richard.chen, raymond.chen"
  echo ""
  $cl transfer masteraychen $con "100.0000 MGP" "list:2000"
  $cl transfer richard.chen $con "100.0000 MGP" "list:3000"
  $cl transfer raymond.chen $con "100.0000 MGP" "list:5000"
}

function vote {
  echo "3. cast votes..."
  echo ""
  $cl transfer eosio $con "300.0000 MGP" "vote:raymond.chen"
  $cl transfer masteraychen $con "60.0000 MGP" "vote:richard.chen"
}

function r0 {
  echo "Conduct round-0..."
  echo "1) reward w 200 MGP"
  $cl transfer eosio $con "200.0000 MGP"
  echo "2) lists"
  $cl transfer masteraychen $con "10.0000 MGP" "list:2000"
  $cl transfer richard.chen $con "20.0000 MGP" "list:2000"
  $cl transfer raymond.chen $con "100.0000 MGP" "list:4000"
  sleep 3
  echo "3) votes"
  $cl transfer eosio $con "10.0000 MGP" "vote:masteraychen"
  $cl transfer masteraychen $con "30.0000 MGP" "vote:richard.chen"
  $cl transfer masteraychen $con "100.0000 MGP" "vote:raymond.chen"
}

function r1 {
  echo "Conduct round-1..."
  echo "1) reward w 300 MGP"
  $cl transfer eosio $con "300.0000 MGP"
  echo "2) lists - none"
  echo "3) votes"
  $cl transfer eosio $con "10.0000 MGP" "vote:masteraychen"
  $cl transfer masteraychen $con "150.0000 MGP" "vote:richard.chen"
  $cl transfer masteraychen $con "100.0000 MGP" "vote:raymond.chen"
}

function r2 {
  echo "Conduct round-2..."
  echo "1) reward w 200 MGP"
  $cl transfer eosio $con "200.0000 MGP"
  echo "2) lists - none"
  echo "3) votes"
  $cl transfer masteraychen $con "100.0000 MGP" "vote:richard.chen"
  $cl transfer masteraychen $con "100.0000 MGP" "vote:raymond.chen"
  $cl push action $con unvote '["raymond.chen", 5]' -p @masteraychen
}

function exec {
  echo "4. execute election..."
  echo ""
  $cl push action $con execute '[]' -p masteraychen
}

function tbl() {
  echo "show table rows..."
  echo ""

  $cl get table $con $con $*
}

. ./env
cmd=$1
#[ -z "$level" ] && level=0

[ $cmd == "setcode" ] && setcode
[ $cmd == "init"   ] && init
[ $cmd == "reward" ] && reward
[ $cmd == "list"   ] && list
[ $cmd == "vote"   ] && vote
[ $cmd == "exe"    ] && exec
[ $cmd == "tbl"    ] && tbl $2 $3
[ $cmd == "global" ] && tbl global $2
[ $cmd == "candidates" ] && tbl candidates $2
[ $cmd == "cas" ] && tbl candidates $2
[ $cmd == "electrounds" ] && tbl electrounds $2
[ $cmd == "ers" ] && tbl electrounds $2
[ $cmd == "votes" ] && tbl votes $2
[ $cmd == "voters" ] && tbl voters $2
[ $cmd == "rewards" ] && tbl rewards $2
[ $cmd == "rws" ] && tbl rewards $2
[ $cmd == "vas" ] && tbl voteages $2
[ $cmd == "r0" ] && r0
[ $cmd == "r1" ] && r1
[ $cmd == "r2" ] && r2
echo ""
###############   END ###################

