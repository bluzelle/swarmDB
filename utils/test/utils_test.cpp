// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include "utils/blacklist.hpp"
#include "../crypto.hpp"

using namespace::testing;

namespace
{
    const char* BLACKLISTED_UUID    {"12345678-0900-0000-0000-000000000000"};
    const char* NOT_BLACKLISTED_UUID{"e81ca538-a222-4add-8cb8-065c8b65a391"};

    // NOTE - not constant as the test shuffles the list before use to get five
    // random uuids.
    std::vector<std::string> blacklisted_uuids {
            "9dc2f619-2e77-49f7-9b20-5b55fd87ea44",
            "51bfd541-ab3e-4f02-93c7-8c3328daccfa",
            "f06ab617-7ccc-45fe-aee2-d4f5d175891b",
            "507b8661-167c-433c-bc68-0f4b117c5307",
            "7a80b4c1-2d99-471a-a1c3-9978759a00e3",
            "f9739b52-c0c6-4b4e-8132-1f96f896f7a6",
            "acf863ae-9c65-4749-8d4f-f9067966ef7a",
            "694cad9f-a08b-45a8-ac40-1ef71672b570",
            "f51d4e1e-f84d-47a9-9b29-645ecef856b2",
            "0957e369-d794-4b51-abaf-669e50e13e4f",
            "51da6250-5d12-4eea-ae2f-2968039f3445",
            "8eee8cff-1bfe-4714-a418-780cc369bd83",
            "af0a1a67-07a1-45ea-a529-48f5a26bd3ef",
            "ddaa5bea-3380-482b-96b8-cb7e2cbdb805",
            "931731eb-288d-4643-8e11-a7e999169bce",
            "54723428-2f0a-48c5-9b8c-d79df82e4434",
            "adc563fa-4de2-4044-b270-829289c18ba9",
            "112d5c33-5c53-4b59-a177-dfc4ada7be20",
            "e44a220b-f005-4d84-87c4-cd799f29df20",
            "c5e09ed0-d520-4b70-8b6e-94aca1ca8eef",
            "d70749b6-8032-4d96-9c9c-98a288c9963f",
            "8681fe91-f5a1-41b3-bc86-c3ee9fd8cbd0",
            "9db3cc1f-d778-4535-afec-342566b51291",
            "22d60214-8dc8-4e98-ab71-096651694a71",
            "86695947-ae33-4838-99a1-8e46ec7b58f2",
            "ab0bf354-114c-437b-add7-c43c784f05e1",
            "752c39f4-6c60-435f-a64e-37c953f713d3",
            "3694f8a2-d1cc-46fe-86a6-471e5bebbda8",
            "4b00901b-959b-4716-ba10-0bd4ec9370d4",
            "90df5baa-c7eb-45f7-aaf7-7b8f5cdd43a0",
            "ab7a83fc-970a-4bf5-a10d-f50b91caece6",
            "78b6055c-db92-4730-89cf-dc3be293fe24",
            "6ea74f15-167e-4636-a40b-17c7e80ae16c",
            "2b710417-7229-4003-820d-512a71858b4a",
            "059b9c82-a85e-4fe7-804c-a273359c6d2e",
            "5c7181b5-2223-447c-a5c1-1b353b06f347",
            "d5d8ca77-965d-4e45-b86a-d6bcd17fe5ca",
            "064d6286-04b7-42a8-8390-caa3dfa8f5f1",
            "d442c865-a3c2-44a3-8327-654f3c957ef3",
            "94a5a516-4164-4a20-b539-67b5307122d5",
            "0d1866ca-d098-4f5a-8b57-2f39a71ee6ff",
            "5d14a2c9-7ea8-4110-9cda-d274b2029447",
            "ffd88060-0188-4520-a2b9-b2fdc964ad2c",
            "f2956275-49db-438b-a1ba-b59e834b5ded",
            "afd1420e-ea14-453d-bef3-3c8aa1538668",
            "8fbf7f02-7eb5-4af6-bcd8-fee9c80d0674",
            "d2d87576-e1e8-4075-ab7e-c4524184ccbf",
            "61013162-b24e-4d81-a94e-1f7a04541682",
            "8cca1307-47b7-4187-966d-b69f37adca3d",
            "621384ff-b3a3-47ec-b678-12e67f7996b4",
            "1b7ec08b-293f-4d6e-ba6d-53c4e591e50c",
            "e4884015-ce37-4500-9979-ccee97a074dc",
            "e0f82155-5b3e-4b7a-bbbd-c7a99464cdd7",
            "fea17fe7-f476-4745-adcf-5a0c30a8647f",
            "22ead2c4-2515-4dd3-84a8-dec18470b865",
            "13b2a577-974e-4c3a-be99-9944726a8394",
            "079221c5-3f39-4600-abe6-eef2451bc3f7",
            "f310f466-5a41-448a-8c24-34a483fac06e",
            "8571dea6-e3ad-487f-8fd9-14e458b3d573",
            "ae272542-438e-470a-a27e-5042c64f63f3",
            "fca4fbba-b303-4fcc-bbe6-c217e1d7f53d",
            "31bda844-9273-4a22-a4d2-021de9c77d55",
            "a0f833fe-94f2-48a5-92e4-9345cc3fbdb3",
            "cbe0d517-0c75-4126-9c2b-5f59a362b692",
            "b9c765c6-35fd-4382-900d-8ab4099605da",
            "573809ca-c33f-477c-a3c3-4e74e9c1e8ec",
            "df8c6874-f53a-4a84-ad49-9bf741c6e246",
            "920a0d3e-3b8e-4143-87be-c33eb0c68e9e",
            "362ba25d-2a22-40c0-b02d-ad5c771313e2",
            "e01a12b9-62b8-46c3-8fbe-dd1f9440afb0",
            "4bf177b4-dc37-4296-a584-9000756c8232",
            "39d73697-7f04-4234-a0e2-f36ee07da8bd",
            "11f9dab3-0867-4ea2-9424-14c3129d5e38",
            "8e981c03-f1b9-4ad9-ab5e-f8202b8a8e12",
            "62fc6f91-d60b-4ec4-b1a6-5d6dd4d730c3",
            "db551dd6-598f-46b1-8218-e0ea224674b9",
            "acdde094-85c8-4b11-b763-8e73c946744a",
            "15367d6c-b583-4508-a8d8-ecfcdbfc5e57",
            "c5340634-1ef0-4782-bf48-9ca6107ba645",
            "8243eee3-a6f7-475c-8a3f-377f5b47bdf7",
            "4b92c0d3-a409-4779-97e2-d73aa3bc089e",
            "5c916c69-18b4-4968-b943-14ad2e022c51",
            "9bc37129-fcf1-47a2-80c2-bc1ba655dd4c",
            "9a976ea7-677a-47fd-932d-4bf416d8ba8d",
            "71276e61-2c09-4991-8082-6a4361a53ac6",
            "8188585a-6524-45d5-9fce-4b9cc4af9b99",
            "9f5d7454-2cf1-4dbc-9846-5c9ac4feae9d",
            "e083b70b-fa91-4d0e-b0f9-f794ec899653",
            "04a72608-9620-4057-94b1-4982a49af76f",
            "b1a857c8-554f-444f-b289-87a1bcf3d26d",
            "8abbe344-00b3-4258-85f2-684bcf5913f1",
            "eaba2810-3b89-4b75-a694-482c270c098e",
            "8e207267-f10f-4470-ae35-ac35cf8e116c",
            "e42f025b-bb10-48ec-89fe-294ec73b7eae",
            "a25cb4d6-6f90-40f3-a791-1d4128a57fd2",
            "152b3892-c1d0-4c2a-b860-e15414cd3aec",
            "a4c1738f-2b28-436d-ae06-2b290630a2cf",
            "6fc878eb-2341-4485-93a6-5b84e68f2250",
            "63291098-96fb-4b83-8427-f0dc7c921151",
            "0d56fcb3-ae1e-4b3e-b794-be3270cc9d43"
    };

    auto random_engine {std::default_random_engine {static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count())}};

    const std::string password {"6Y4qTvHbQcyNccKW"};

    const std::string private_pem {
            "-----BEGIN RSA PRIVATE KEY-----\n"
            "Proc-Type: 4,ENCRYPTED\n"
            "DEK-Info: AES-128-CBC,5A3EFEE2F79F6DF164C14D7AF76F02DD\n"
            "\n"
            "Za5/QtVvpTi7yoapfVDUnrHSllLisFXyxkCZtyfknoBY558QCUK5HI1DZDvF8ABH\n"
            "mxxODV0MLJjDVe7MMCNwbRqbMTTNXp4wUe5RcyFVKdBI2v4aloD3JVCuua/1JRwX\n"
            "foZz39i/PCf8h7/8hhGbaJOP1G8cK4j841rO2OTZzCAJegPYLjdxhUmU8N4ihH8/\n"
            "pSaNdhnRsT6Abv3cDIPb8/PSYIVJmtnA/uINNb1aKlPWVimGO24DX/ZzDf6/THQl\n"
            "oi2kP11W87Uz5KNOw6Vo/0bemBOtJ6tVvBtHY+WqwBkUszIW/AnfmA1EA9w4EO95\n"
            "qYY1sV97+R5nZ0wl7ldXgUVXYTmAIGjAOxzcIcpsf/r/Vx1dWjGE+N72hs6289o9\n"
            "OfBtj/6SfwCpEOVSZ6yE7h2Q9rlW6qHw3Nk7ycBtI2Ksh8JCEEgg1DC/7h3EwSca\n"
            "M+CDQjHzgSnv6HhT36zAxXaWVvhSjoKv6bbBgHraZYtvE5qsHwgr+S44cCJIJJky\n"
            "wOUnxOGBQekn90jd7xsgK8QzbIszH5yN48ZJYpUpZaQqtKu4DH6hh64LjvAblVkt\n"
            "E7kDhdY6PDXlhnF2ZzH9inL1q9zxNez5V37zVkM0v1W+TiPgG9WfAZdRtywcv9hN\n"
            "NcRyTSKCqwEGQgKXPXcF8rwbHfn3HlnAJmNDrudRvZuhfQKfKGqn/VC4CzXIBIxW\n"
            "M/PAszY3WI9YqsCnnpchcXmhi+VAftbLxVC2Cbcfxlblu2+fOk2OiYzadK1i1PVx\n"
            "W2xrLa+BAkubuT3XX3IYwboZTEEGFm/uOESC0nYjBen7/d9EToSESA3lgT0lVXEV\n"
            "jVIRSue0wAZRPejisTEnAyzosRQkrCZUSV2p5O1dJ3WpEpDMyU4V+y4kxVWPJaRy\n"
            "JntQCddjfml02uELqIppA/n2DV4f53j4P2usmwT81Gw+b0DaxRDXXp94HD8eNzup\n"
            "70vgYZZeQU8RBqSij1s66nR5msUKn5yW+qSmQyNneqlgyloQfyav9ZSLQbopjZcg\n"
            "yttYVskMNIhTT0QBuiKFMbJJO6gSYxtFViZ4Vv6+t6jARkAMHJDOkUvOFn79sSM9\n"
            "2x64YcXZJUsFs31d3VZUYt+adyAIIT8WT6Ig0xBOPPVk8BKAva79SP51QXRPfxzw\n"
            "8KrB56zUzMAO/cPfyhaRHo/i+Pb1jII747UQ5/ELGym2VQU0CZvCueUdQJIWNU54\n"
            "IaXbAYdhCAQbd6nj0GH2R/S3Q0qwI2ufb3zVVCSxmo9CSjkioOJ6gQ11n1KwpvyH\n"
            "dyI1nvioQeKPAWR7ipiY3gnlgVZeemRbjrfoSwvUvWx4e8KQlul0iHCZJkmPLSuU\n"
            "mPsd+qpjGs/R59r6J7RUqTXadqqTb6pAKMt3MiXxjsvdZwJqNWDbpbaPgLmkIc84\n"
            "0+giSG58T7EgDZk0Mnu3TgMKtLwggIRwEzvZF1ljx6xyTuBlRCZrw9PsDfI15xZs\n"
            "K3OzOT7EPkdyimpsqNVuLpMqflItgaexM3C3d4syarCni81fce+mv4jA5Bb6Lh77\n"
            "uM9UaSooARR9hlqEms6ZCGcfWjDGOQrz8JnM1gWmC6wjYYFu4xWZzeOg+YmECCtY\n"
            "/Txry/4I/kkC5BO19sdNx5L8tsflkeiGjej2syXk/ecsJ7db0ZoJ56EeXX0TsIUw\n"
            "O8vnhGwEOiNyJNjnvxzE8GyY6VlS7JtORl4KxcIBF5dRcoeVTqzo+l9zhM3ZhrLr\n"
            "NRlTmVMEEGEafINI+VBz8b8yT4EJzc1ddyUyipK60SXYfoj1rqoVIxcauFSndVyM\n"
            "7p4CctbAjMgE86cq01JAIEJQEAouEeJeC12qdUbM1R69JTFSF51wqeiRqoXrltD5\n"
            "uQHcOc7OnUSTdBACnWh9UZ3Fo22H3ARbdOoCDo5KtB8jwCPzNyTuZjYpL6iYOtjf\n"
            "trPy7FsjCEW2uINemFIplBMGvFPhmHm7N/XU1MEdbYrijsfgqfGZaUrGMnj8oKx0\n"
            "ZCfM8rhPlW7l9sX0b3nEWTZ0q6IF9a28sAJe+/7mMBuqCrDSIBfU7BQjji33ODEH\n"
            "VpelBl5jPWS2gSw/a4LKZ+muOAPXIm3SfNnmSiWCaG/qMD3LhyfQ6fOgDapKdbKy\n"
            "8hgGMfxW9+2qXOnnTeg6PCca/zNyUQeiEgom9oNmweqDBGFNP0Rz7zRN9pWuGSaM\n"
            "DIxtnYBQ4m0dgcdGMa76Ts15ojehwdjdWySdYgfl3z2kMOVEcL0an+wz4bNgBVjN\n"
            "MAtmt1XHa2XBor4R8nENVtA+f8ra6edReVxnTQxvDY5b/WOy9zgOMdp9OIXJ2YXY\n"
            "RoHYntAQIDR3y9phCFRra1ZGkT/tCiDwz8MqTnmo9sFbYW/LC+yU0kmZfWrNj7SH\n"
            "zYGSBBxUAMD57fVuThYD8PAk6w5BHAxm62B7AzmQvQHw1XCXZxoCKIjPHbgFzVJX\n"
            "oc0+WsBee7LdzDYcU3eKfYCseQBS65AwK/IPqeIFbkHGblCGEY08Ju74UjE5+yi2\n"
            "8lbKnh2frV6Tgp2niJ5FDhIupV9lx0p2AnLj1ZDTNZAH46O4PZEghrZYda/Kq1qJ\n"
            "9JCKeijI/f1nCRx5sZbdzsSN8YjznAl0FuivNyGdhP2yu/+DSeUZCySrTBNd49cF\n"
            "yhVIGcd7prqAoa5LrJNwNyVWtznvK7cC668Mrag9T4vIUDRYmUzPnf8YPMsq+aod\n"
            "ZTNQEp1HM75xiFAHXLoZJV3UR5ixcRDzKVCLbLMX0ghXc2Kx7vjrEoJtXgAzI6Mm\n"
            "xD19xEhYOb9b/fMPSxxzoPXV+kQ3E55lpN1TLh7vxSDLh1pZWCqZcp3uetuwTMeR\n"
            "3+DKlgTvIcID3tyc5OFNWLs5gyTSIetGB+BY3qLQHif/UqfyMzDqaWLhdEMUcXXW\n"
            "j5PV1NdweDUwGhScJZZYFwt+nErUqvV2fBkQE7lhAv+RPL+M3Gg1eD4H56dxXPrn\n"
            "7BeRnyEcK/i+c6I9/0ylF4Uz6by20gMwSIQV3lJZP/Uvr1WuFLGMNdlQI1R0wSpN\n"
            "zfcD8egPqjuSTRjyWCw15k4kGoP4psK0k90Pdh9kLsK+HfKKwjuPyGBx8CiFKNrQ\n"
            "hCnBxLcxJoIffeSzVr/7c+1dVzmlCYLX0Vbv3V5jJMfFKfXcDVHZMdHiZut9Mwk9\n"
            "-----END RSA PRIVATE KEY-----"
    };

    const std::string public_pem {
            "-----BEGIN PUBLIC KEY-----\n"
            "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA1KVskYjJo1kMmLPcf1dA\n"
            "+OxTQD3Rs2ivNOfZe7z8+SZXVEeGXuUAF7qLcza16smpQno2mAL/amYf+BSWuhQf\n"
            "jCDWVH/ypSg+C0/8Cx0Qksi5Y+Shopq6PJx0aONLUWtuD6bfGLhBylT6GV66aoN4\n"
            "ozD5mUbPd8/p/5KdLbWTKhCJSqiioa7VmObTegs+CPnCF1KWyLWHSKidZr5ufm2D\n"
            "wM6IpHIW3ryGCkHSlalV/z2IzSlw+8bVfcQ2+7ZZx+BLmtYd0oxnu2oHPXCKMktK\n"
            "mKqZV4sSuuOyan7b/nm4nUubCaJYUjoSDRxJ81Ppo94GSc4r8ga4qZShPPHDgSBJ\n"
            "b3phnOxMdFYcgRTmL7pqobsRiQFJcQgRgX92/vbsZZx+p6axHbT1upBEKxFhn32o\n"
            "hSUycb6ozTP2vyB5S07wSlla5Grcv6JIVJFbTn5Gw3Je54hMaFVqIKcX4eK5cYZ6\n"
            "sFrn8c+G/WGC1S8GcWa+7PdMMCSWM9XQjnLoZ8qh80WUfsUTphbdo+ABherxcpeO\n"
            "P1jN0qg/JNAGAQ9n5BaOoUWBjyN8+oyZ7fnwd4ZGkAJL4XkyGnoFxakVZqtVQGbV\n"
            "9JBIbFkLtuloHbo0Ax56SDvQ1WDTiusYkkLeU0uUXHcFKa+jcbhjzYqHLW5bA4Mn\n"
            "g9rJ8ehWRHw3TuXh6EX4xTcCAwEAAQ==\n"
            "-----END PUBLIC KEY-----"
    };


    // note the \x0a appended to the end of the uuid's. Since the OpenSSL signing
    // is done on the command line with text files, the \x0a is the new line
    // marker and may need to be appended for the signing to work.
    const std::string valid_uuid        {"9dc2f619-2e77-49f7-9b20-5b55fd87ea44\x0a"};
    const std::string invalid_uuid      {"8eb1e708-2e77-49f7-9b20-5b55fd87ea44\x0a"};

    const unsigned char signature_sha256[] = {
            0x0e, 0x9a, 0xed, 0x6e, 0xb0, 0x5f, 0x0b, 0xaa, 0x57, 0x47, 0x18, 0x94,
            0x08, 0x14, 0xe9, 0xbd, 0xf2, 0x2b, 0xfe, 0x00, 0xdb, 0x9e, 0x38, 0x54,
            0x22, 0xb4, 0x32, 0xb2, 0x7a, 0x97, 0x34, 0xbd, 0xc7, 0x90, 0x6a, 0x8d,
            0xd3, 0xb6, 0xaf, 0x43, 0xd7, 0xa2, 0x97, 0x3e, 0x13, 0xd3, 0x9f, 0x05,
            0x1d, 0xaa, 0xe2, 0x2b, 0x92, 0xf9, 0x57, 0x6c, 0x68, 0x52, 0x7d, 0x9c,
            0xf4, 0xae, 0x6c, 0x29, 0xc8, 0x78, 0xa8, 0x6a, 0xc2, 0x61, 0xed, 0x91,
            0xb3, 0xbf, 0x9a, 0x96, 0xc8, 0x83, 0x06, 0xca, 0x87, 0x66, 0xfa, 0xf0,
            0x41, 0x06, 0x5b, 0x49, 0x19, 0xf3, 0xea, 0x22, 0x8d, 0x8a, 0x37, 0x25,
            0xf2, 0xb8, 0xea, 0xb8, 0x17, 0xaa, 0xb1, 0x1b, 0x61, 0x2d, 0x34, 0x58,
            0xc4, 0xb1, 0x6f, 0x79, 0x5e, 0xdb, 0x36, 0xc6, 0x98, 0x7c, 0x5e, 0x23,
            0x3a, 0xc7, 0xf2, 0x43, 0x02, 0x9e, 0x43, 0xb8, 0x49, 0x5f, 0x7a, 0x47,
            0x85, 0x89, 0x10, 0xca, 0x1d, 0x64, 0xf1, 0x6a, 0xf3, 0x01, 0x68, 0x6b,
            0xd3, 0xf3, 0xdd, 0xa9, 0x23, 0x93, 0xf5, 0xf0, 0x60, 0xae, 0x83, 0x68,
            0xd1, 0x34, 0xe0, 0xdd, 0x10, 0xd2, 0x27, 0xa2, 0x3b, 0xf3, 0x14, 0x7b,
            0x81, 0x20, 0x58, 0x13, 0x90, 0xbb, 0x3c, 0xd1, 0xfa, 0xdd, 0xda, 0xbc,
            0xab, 0x0d, 0x96, 0x3e, 0x2f, 0x95, 0x12, 0x51, 0xc1, 0xb4, 0x90, 0xfe,
            0x12, 0xcb, 0xb7, 0x64, 0xc8, 0x87, 0x9c, 0x29, 0x86, 0x3c, 0x4c, 0x75,
            0xa7, 0x8d, 0x2f, 0x2d, 0x28, 0x95, 0xcf, 0x9c, 0xf7, 0x24, 0x52, 0x61,
            0x21, 0xbf, 0x45, 0x7d, 0x8d, 0x51, 0x19, 0xad, 0xd1, 0x98, 0xd0, 0x5d,
            0x3e, 0x21, 0x3c, 0x3e, 0xf6, 0x2f, 0x14, 0x6a, 0xe6, 0x2e, 0xc2, 0x83,
            0x1c, 0x29, 0x4b, 0xdc, 0x2c, 0xd4, 0xc0, 0x2e, 0x16, 0xfd, 0x18, 0x45,
            0x72, 0x83, 0x0b, 0xeb, 0xff, 0x92, 0x35, 0xac, 0x2d, 0x6a, 0xa9, 0xa3,
            0x27, 0x80, 0x00, 0x4b, 0x25, 0xa4, 0xb2, 0xd7, 0xf6, 0xda, 0xdf, 0xea,
            0xa5, 0xf0, 0x18, 0x91, 0x70, 0x92, 0xa2, 0x1d, 0x50, 0x0f, 0x8e, 0x89,
            0xc1, 0xa3, 0x8b, 0x1c, 0x4d, 0xcb, 0xda, 0x0e, 0xf0, 0xa1, 0x44, 0xf7,
            0x10, 0x56, 0xb6, 0xce, 0x26, 0xa1, 0x90, 0xb2, 0x52, 0x00, 0x37, 0x65,
            0x99, 0x1d, 0x59, 0x29, 0x11, 0x40, 0xd3, 0xd9, 0x7b, 0xfa, 0xd9, 0x04,
            0xbf, 0x6a, 0xde, 0xcf, 0x43, 0x4c, 0xa5, 0xe5, 0x17, 0x5e, 0x8f, 0x80,
            0xfc, 0x91, 0xb9, 0x8c, 0xc2, 0xae, 0x25, 0x5b, 0x49, 0xac, 0x41, 0x07,
            0x2a, 0xe6, 0xd2, 0xa7, 0x1a, 0x40, 0x6f, 0xd2, 0x92, 0x6e, 0x9e, 0xa7,
            0xac, 0xa1, 0xe7, 0x01, 0xe2, 0x4a, 0x25, 0x6c, 0x98, 0xf9, 0xa1, 0x35,
            0x2e, 0xf2, 0x81, 0xa8, 0x23, 0x76, 0x3d, 0xc6, 0xa4, 0x2f, 0xa3, 0x27,
            0x4c, 0xc7, 0x5a, 0x56, 0x00, 0x12, 0x87, 0x04, 0x8f, 0x9a, 0xd4, 0x04,
            0xe5, 0x9f, 0xa6, 0xf9, 0x64, 0x49, 0x72, 0xc1, 0xc3, 0xc5, 0x53, 0xda,
            0x48, 0xae, 0x01, 0x40, 0xec, 0x78, 0xab, 0x85, 0x36, 0x7c, 0x72, 0xaa,
            0x13, 0x97, 0x5c, 0xd3, 0x96, 0xce, 0x0e, 0xed, 0xa4, 0x73, 0xf4, 0x2d,
            0xfc, 0x89, 0xf7, 0x74, 0xbe, 0xa9, 0x1e, 0xa0, 0x93, 0x1b, 0x25, 0x16,
            0x9a, 0x06, 0x71, 0x8d, 0x43, 0x2f, 0x00, 0x9d, 0xea, 0x37, 0x1b, 0x8c,
            0x8c, 0x9b, 0xb4, 0x17, 0x57, 0x7c, 0x2b, 0x13, 0x12, 0x49, 0x14, 0x3d,
            0x38, 0x2f, 0x8e, 0xa4, 0xe2, 0x0f, 0x82, 0xc8, 0xaa, 0x2c, 0xff, 0x05,
            0x0c, 0x46, 0x1c, 0xa9, 0x20, 0xb4, 0xa7, 0x6c, 0x2f, 0x6c, 0xca, 0xed,
            0x6c, 0xbd, 0x55, 0xcf, 0xec, 0x46, 0x24, 0xb8, 0xb4, 0x86, 0xbc, 0x86,
            0x0f, 0xa6, 0x8b, 0x03, 0x71, 0x2c, 0xe2, 0xf9
    };
    const unsigned int signature_sha256_len = 512;

    const std::string signature {
            "DprtbrBfC6pXRxiUCBTpvfIr/gDbnjhUIrQysnqXNL3HkGqN07avQ9eilz4T058F"
            "HariK5L5V2xoUn2c9K5sKch4qGrCYe2Rs7+alsiDBsqHZvrwQQZbSRnz6iKNijcl"
            "8rjquBeqsRthLTRYxLFveV7bNsaYfF4jOsfyQwKeQ7hJX3pHhYkQyh1k8WrzAWhr"
            "0/PdqSOT9fBgroNo0TTg3RDSJ6I78xR7gSBYE5C7PNH63dq8qw2WPi+VElHBtJD+"
            "Esu3ZMiHnCmGPEx1p40vLSiVz5z3JFJhIb9FfY1RGa3RmNBdPiE8PvYvFGrmLsKD"
            "HClL3CzUwC4W/RhFcoML6/+SNawtaqmjJ4AASyWkstf22t/qpfAYkXCSoh1QD46J"
            "waOLHE3L2g7woUT3EFa2ziahkLJSADdlmR1ZKRFA09l7+tkEv2rez0NMpeUXXo+A"
            "/JG5jMKuJVtJrEEHKubSpxpAb9KSbp6nrKHnAeJKJWyY+aE1LvKBqCN2PcakL6Mn"
            "TMdaVgAShwSPmtQE5Z+m+WRJcsHDxVPaSK4BQOx4q4U2fHKqE5dc05bODu2kc/Qt"
            "/In3dL6pHqCTGyUWmgZxjUMvAJ3qNxuMjJu0F1d8KxMSSRQ9OC+OpOIPgsiqLP8F"
            "DEYcqSC0p2wvbMrtbL1Vz+xGJLi0hryGD6aLA3Es4vk="
    };
}


TEST(util_test, test_that_is_blacklisted_member_returns_TRUE_if_uuid_IS_found_in_blacklist)
{
    EXPECT_TRUE(bzn::utils::blacklist::is_blacklisted(BLACKLISTED_UUID));

    std::shuffle(blacklisted_uuids.begin(), blacklisted_uuids.end(), random_engine );

    std::for_each(blacklisted_uuids.begin(), blacklisted_uuids.begin()+5,[](const auto& uuid) {
        EXPECT_TRUE(bzn::utils::blacklist::is_blacklisted(uuid));
    });
}


TEST(util_test, test_that_is_whitelist_member_returns_FALSE_if_uuid_is_NOT_found_in_whitelist)
{
    EXPECT_FALSE(bzn::utils::blacklist::is_blacklisted(NOT_BLACKLISTED_UUID));
}



TEST(util_test, test_that_a_poorly_formed_uuid_fails)
{
    EXPECT_THROW(bzn::utils::blacklist::is_blacklisted("0}56fcb3-ae1e-4b3e-b794-be3270cc9d43", "http://localhost:74858"), std::runtime_error);
}


TEST(util_test, test_that_openssl_based_base64_encoding_works_correctly)
{
    std::vector<unsigned char> base64_decoded_output;
    // TODO: return value for success is 0, in this case. We should standardize this across the rest of the application
    EXPECT_EQ( 0, bzn::utils::crypto::base64_decode((char*)signature.c_str(), base64_decoded_output));
    EXPECT_EQ( signature_sha256_len, base64_decoded_output.size());
    EXPECT_EQ( 0, std::memcmp( signature_sha256, base64_decoded_output.data(), base64_decoded_output.size() ));
}


TEST(util_test, test_that_a_uuid_can_be_validated)
{
    EXPECT_TRUE(bzn::utils::crypto::verify_signature( public_pem, signature, valid_uuid));
    EXPECT_FALSE(bzn::utils::crypto::verify_signature( public_pem, signature, invalid_uuid));
}


TEST(util_test, test_that_verifying_asignature_with_empty_inputs_will_fail_gracefully)
{
    EXPECT_FALSE(bzn::utils::crypto::verify_signature( "", signature, valid_uuid));
    EXPECT_FALSE(bzn::utils::crypto::verify_signature( public_pem, "", valid_uuid));
    EXPECT_FALSE(bzn::utils::crypto::verify_signature( public_pem, signature, ""));
}
