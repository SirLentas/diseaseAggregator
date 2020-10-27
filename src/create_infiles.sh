#! /bin/bash
diseasesfile=$1
countriesfile=$2
input_folder=$3
d_per_dir=$4
r_per_file=$5

k=2
while IFS= read -r line; do
    echo "$line"
    mkdir -p $input_folder/$line

    i=0
    while [ $i -lt $d_per_dir ]; do

        day=$((RANDOM % 30 + 1))
        month=$((RANDOM % 12 + 1))
        year=$((RANDOM % 20 + 2000))
        date="$day-$month-$year"

        j=1
        while [ $j -lt $r_per_file ]; do
            n=$(($RANDOM % 9 + 3))

            first=$(tr -dc 'a-zA-Z' </dev/urandom | head -c$n)
            n=$(($RANDOM % 9 + 3))

            last=$(tr -dc 'a-zA-Z' </dev/urandom | head -c$n)
            dis=$(shuf -n 1 $diseasesfile)
            age=$(($RANDOM % 120 + 1))

            pos=$((RANDOM % 2))
            if [ $pos -eq 1 ]; then
                echo "$k ENTER $first $last $dis $age" >>$input_folder/$line/$date
                k=$(($k + 1))

            else
                k2=$((RANDOM % ($k - 1)))
                echo "$k2 EXIT $first $last $dis $age" >>$input_folder/$line/$date

            fi
            j=$(($j + 1))
        done

        i=$(($i + 1))
    done
done <$countriesfile
