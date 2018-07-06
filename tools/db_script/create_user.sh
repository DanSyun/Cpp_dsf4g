DBIP=127.0.0.1
DBPORT=3306
DBUSER=root
DBPASS=root
DBNAME=user
TABLENUM=512

IN_FILE=create_user.sql
OUT_FILE=/tmp/create.sql.out

tableid=0
while [ $tableid -lt $TABLENUM ]
do
	cat $IN_FILE|sed "s/\#DB\#/$DBNAME/g"|sed "s/\#TID\#/$tableid/g" >> $OUT_FILE

	tableid=`expr $tableid + 1`

	echo -n "."
done

echo ""
mysql -h$DBIP -P$DBPORT -u$DBUSER -p$DBPASS -e "source $OUT_FILE;" 

rm $OUT_FILE