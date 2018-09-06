#!/usr/bin/env bash


usage()
{
    echo "usage: sign_uuid -k path/to/private.pem -u <uuid_of_node>"
    echo "The output will be signature.txt which can be provided to the node owner."
}


write_uuid_to_file()
{
    uuid_text=$1
    uuid_file_name=$2

    echo "Writing UUID($uuid_text) to $uuid_file_name"
    echo $uuid_text > ${uuid_file_name}
}


verify_input_files()
{
    private_key_file_name=$1
    uuid_file_name=$2

    if [ -f $private_key_file_name ]; then
        echo "Found the private key file: $private_key_file_name"
    else
        echo "Exiting - Could not find the private key file: $private_key_file_name"
        exit
    fi

    if [ -f $uuid_file_name ]; then
        echo "Found the UUID file: $uuid_file_name"
    else
        echo "Exiting - Could not find the UUID file: $uuid_file_name"
        exit
    fi
}


use_private_pem_to_sign_uuid()
{
    private_key_file_name=$1
    uuid_file_name=$2
    temp_sha256_binary_file=/tmp/sign.sha256

    openssl dgst -sha256 -sign ${private_key_file_name} -out ${temp_sha256_binary_file} ${uuid_file_name}
    if [ $? -ne 0 ]; then
        echo "***  Exiting - Unable to create the binary signature file"
        exit
    else
        echo "Successfully created binary signature file.."
    fi

    openssl base64 -in ${temp_sha256_binary_file} -out signature.txt
    if [ $? -ne 0 ]; then
        echo "*** Exiting - Unable to base64 encode the binary signature file"
        exit
    else
        echo "The signature has been written to signature.txt"
    fi
}


############
### MAIN ###
private_key_file_name=
uuid_file_name=/tmp/uuid.txt

# TODO Move the while block into it's own function
while [ "$1" != "" ]; do
    case $1 in
        -k | --key )        shift
                            private_key_file_name=$1
                            ;;
        -u | --uuid )       shift
                            write_uuid_to_file $1 $uuid_file_name
                            ;;
        -h | --help )       usage
                            exit
                            ;;
        * )                 usage
                            exit 1
    esac
    shift
done

verify_input_files $private_key_file_name $uuid_file_name
use_private_pem_to_sign_uuid $private_key_file_name $uuid_file_name
exit 0
