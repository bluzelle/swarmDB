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
#include <utils/blacklist.hpp>
#include <utils/crypto.hpp>

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
            "DEK-Info: AES-256-CBC,4C0EA37BB36FBF5E9799A14A3EC78A70\n"
            "\n"
            "srl0+9zecwEzGrWkZXMexv6JaxL+O1B2N2xwXrFcDpcish/2MiRWybj1vT+U3mEq\n"
            "B5AweV7OCeDBpoqxM6cN9mYOVSmBdHxf2Ek/RGi2/o2r1nf18s4BTxupReauaJOu\n"
            "yJiNgOMcRE7a30SGwguJmNf8XkPIjitrPNLcgQv3q4GTcXV4vrnElnyuINBWrg2F\n"
            "wl/24/Pxdw4OEGik5FN1IjY1GhhnwBLn36H9FToRaj/id/vBpZ0cFIP/AbHJto2H\n"
            "OoIdvG6xIt6pXuk9WhPAOVbxeYy0Q5C1vLdH22OSTDe+3wiZpRIkllZBMjdLHl5X\n"
            "pdxmG4YwPjZIgM5jTZRBRrYDyZHroJqlJFEGnvDLuPiy7TlVA47oZGKKLsga+aFQ\n"
            "uZKJ2duANgMiM6re3WFePw6+O9YjRnhNjj70S3YM8l5cCRUFkOeLjFxGEDtjndV5\n"
            "903du5LdzPpneAbehPFMafR2T8WeKKp2b4Ulpxw+x4klDpMRL1n0ow4rRRcFsIm1\n"
            "wh/p7Mr8DvDqChelhSkksqfrXfkGg1kFRGMmS1ftQ8tcL7pZM/ZDXhpxYXzSfY7p\n"
            "ZqlWd3e9NRybT25upQOkkW5IFM8qBri0tQ+hcI0ZV7yFiCdyN0K3r+dlZCHso8BX\n"
            "4d99LWPKW7Y0vt2za6OnD/nKLfRHEUIL588eLRQskID9jhPyLlPjRGHOmDK2BF1m\n"
            "/dcuHvBWdwxPXKeDWvAGrygrMTVs0Lz9Gn2gMO6XFqVSlLhCEueggksfSjjgRvPN\n"
            "1nGIWeHHyF4LKRXNV0Fstt26H8O1I9WTBozFC7MtsdIORC3OinftbTuc7dZeb110\n"
            "NJpCLWePFT5mIzEqAKYwxkMPLGLHfH5CzJpmNFaN+4UmQiOwwNiigjFLLykjli1C\n"
            "g/3Tk32E4uADff1xzIb5+hV28Yh2I45np7YXJX5FowWoGQKL8/qCs/BhwA6rHTkw\n"
            "w7W/G/BLjwEti9jwWO4XnI9PIRm3rzRmr34J4+x2qSKYi41LajiEdk00sMxcHupx\n"
            "SteCY0IHsSL0P7DVHfkjUV/rx8GLh7ugb/TMoiS3Dvh6Tk4Dj6cW7nRSYvFALu0v\n"
            "iofcr89K/gbkyOms4rHdd1/124zMRzh3KZs4O6ZnEFgTTEVMMGYrSS6gpvKHyp6G\n"
            "CCEZHDe2mZHuOI64iEDXyX0qvCLBBrMUBu/zeQ44zf0AtI4vkROSNR9TIY59p93s\n"
            "+a51iW50gTuv9Vojd6jHzKPMqe2PrKGekFZBTpROQsUBiPepTDT00lkb+u7t4sYp\n"
            "wbWwQlvEm7JVrK7HxP8/dvz19CVs41rEM9op2AT3Pg18jG+LXZfbVDX/Yv4P8077\n"
            "IB4XMAKh/CazkTp5H3lIujcs9LKS/jtCsnSUGu5myS4rr3XH9NSYN2LUmHfHOInW\n"
            "YMHGRkNkBT7Znt/khVpwENeC0x1nT6c7V/bgQ3mV/xsvE3utKsNi78P2VOippehA\n"
            "npG41/cfUSI8yFWMQpmAezgFeLAojM0j71l0ZQqdcpvnUl8+wBnOSzEtsx7XU6of\n"
            "UXvsuDyWJkyYe9bHchyKYlstYlnmaIGQu1uzpjix5m2WHhdHHrgTsF7YJ19UU/TP\n"
            "gE5cuuMpmuRTQRCii2kh6YghsOEKF0+C/zg2wAHLuLXOZzGLBcEvxHL/0U/5s6Yg\n"
            "5HiUeW5FtnfNO8TvvVFT0odTK5lfIqqRn2td1w9Q7q7sVPgGu35P6Lb7tP31Se8N\n"
            "CjOmcnjMtFH/PV9yzq9mmhsroHDmWeF0YIl0ghdYh3fDFo6StDl7WzTNzrqQdvTn\n"
            "aetSA5odZ8m+Us7PFXWGijedwBfdvWdljQlw39vQ6voQiCZY+drcyeXEm3YAUtgD\n"
            "3m6qMiZ2D86O3SLBm2AJaVlQZH2nIQm1UJZdjZNYW5y3fvTjIBjw6Q2WfhXub5M2\n"
            "OJq40X+7AkKunKj0zmsn9TRaoRnasmxh542Fy1pwTpll9pR7DZVSC6mtMffOSKz4\n"
            "XJr3CX5j8wUMnVSuISYuW5innPoZIOQ5gKFtpdGUTpaxD7ZFxU1wyzycmFZ9ZGGJ\n"
            "9f72DfAYHFNMMM4ymh2VZ474OFmx9ewPIWxNm4svV3OiPOaAVMnw4+3Jdxp92ivb\n"
            "YK3j3vnyP7fN02CrujAR2RufIUiPOM5NKgJe6mzrOANr/i77Q3A3hiD4tAEWRRNF\n"
            "W+3SuoAqeS4AtTBv2WhQmBXh7PlsfZyjKex/VbyOdNjinvoIMirdSuluirTo0Crp\n"
            "ySZEllKFniZwo/wVJfqhrn3KRYdjO1G8ZkW67MhweHSSbZtqyvzR8OntMkWH4Kv2\n"
            "m+g2vq/OIm4ScjCUUGwm5Kd+f056ec5/78Cxre2VVDu7JHt2kVkAHDOdXvPQHWvq\n"
            "tel83ImadoOhjCZW4aV8xTzvFuRlBNdqEjlygQca5obUdMYB9KAxIUEyyZ2R2L1W\n"
            "of+b3iejYF6h30pVrez3sYdELj5vjAO3wIRlFLtSw7JQgU6m5y0koQTs3KuFQO5n\n"
            "3pFZsld6JAHsLuZ5kh3fwUAxwLaCjfOX7KiFsnBEathEVD8ATvrbDomtkJWOSfYO\n"
            "s4pKrnA1opoSJ7vPt+PsEZwLi4Yki/AqDHAXRnpdKCU1EyumgklGZXwI0scc7Dit\n"
            "MJR5MqWAF46TmsOpowB+rBXXdQi2UgNjBzFjxYa9YRf5sRwvyo0JUcMATwy75g2P\n"
            "yNp9cvNNltRXv3VE1oYxRbeJ9uG0Nf4osq+d/cFUhHVaC610oHTXPou0llpAWefJ\n"
            "lP92HmPyTwIfXtOfYt5Vbv6NOTc2m+xn1w+H8MVniQUbo7JhYs5BBUadc0VScaOA\n"
            "ScBTDWQb/wFqzMQhsaYVNpzMTYyDKxbb63poz7EbbgUb9IBSNvmnIJI6C3ItVDqn\n"
            "xWnTDQ3pk5vdGqgRE0MCrSkzQ3PEK5mlp9wUkqK5Kiq5loIrB19IwXH140dCOd1M\n"
            "vo4o5Ed85CfBl3xFQArFQm+0pQ2oQnU1NHjORHoN1MklAmtmYaFo5JEGHbxQnurt\n"
            "b+XI03vg1Bro8wuaGlSmnktZnKfTBT1FZTJIkFGViY4fFXyM9GL6scEN8nJFgBn0\n"
            "2eeaax2ZXB0ptbay79gkVVVshW02lStYtbyaMC1U/ns48DaUtHknqR72WDt9D1bC\n"
            "-----END RSA PRIVATE KEY-----\n"
    };

    const std::string public_pem {
            "-----BEGIN PUBLIC KEY-----\n"
            "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA3Delbh36s+NJCCOYi1ql\n"
            "NGp+R5EoKWtcazi+Kh/t2V4kN4QCEQdxi3nlhbHdiWWNi8puwwJYFDRdjZvEO+2H\n"
            "yEyFui4v9Rl/RoGdDQeXEeQ+QxMVvT6Ya7+unRDjlIuNmQNNe4HKlcA4fqhRqi07\n"
            "bF2b1kceuPsxIXcBnrsVVqxvSvqjiVkaSnk+HACm8fjiTwGSFE3ZhooMgxENEWrw\n"
            "Ltcr80UdWpMvCrlBCWtlxiLl8VgoPCDZ/R2iXvJaQAOwepfdFGmcPomcO9BOEJfn\n"
            "Jg4sBQNu4CqN5KRCS8CN39E705s4upFMRleU6nKHa0kSb9OsqJP/0i0fBVFLsxpX\n"
            "WbR2iLwpCy3YqJqwoS8PFz9rr4V9ESqRJlczjkRt6bfx4/fSskxsXWRl+Dlv1V/P\n"
            "Sl0zlJXMoxpPuLANxEVsnR9fT07h0XuQ58dsjbMImTL8Hmomfl3n2WK35TesAAaO\n"
            "WNa4+2LFYzKwLUL9cRv696544eo8TCi+bXEKSCmUzpLsLIeHVmeHy3CiUUq13ktE\n"
            "ykQD9vdtoyJkWJ4n5QMQVaV1k1hJ0V8EMfpV9mMEpItfQQRqDW1QG2fHbZgrUV1m\n"
            "5PdnWwf3L9nMNxja7nX0vk5QVw1r8Kl4KqP4sid4djBYz4SBLeSfimUDw/USsKik\n"
            "MwoaYGgrejBmlvZ5UiFw+xMCAwEAAQ==\n"
            "-----END PUBLIC KEY-----"
    };


    // note the \x0a appended to the end of the uuid's. Since the OpenSSL signing
    // is done on the command line with text files, the \x0a is the new line
    // marker and may need to be appended for the signing to work.
    const std::string valid_uuid        {"81ca538-a222-4add-8cb8-065c8b65a391\x0a"};
    const std::string invalid_uuid      {"71da427-a222-4add-8cb8-065c8b65a391\x0a"};

    const unsigned char signature_sha256[] = {
            0x3a, 0x8f, 0x19, 0x94, 0x34, 0x1c, 0x32, 0x56, 0x45, 0xe2, 0x1a, 0xa7,
            0x84, 0xdf, 0xf6, 0x0f, 0x3d, 0xc5, 0x61, 0xaa, 0xd9, 0x1e, 0xb1, 0x9f,
            0xfb, 0xce, 0xe2, 0xf8, 0x95, 0x12, 0xc4, 0x1b, 0xb6, 0x18, 0xa1, 0x64,
            0xf1, 0x26, 0x1c, 0x0e, 0xbc, 0x23, 0x73, 0x40, 0xee, 0x85, 0x40, 0xb1,
            0xa5, 0x15, 0x50, 0xa6, 0x93, 0x24, 0xe5, 0xf8, 0xd9, 0xb4, 0x9d, 0xeb,
            0xe8, 0x8f, 0x74, 0xeb, 0xa8, 0xdc, 0x12, 0x92, 0x4f, 0x96, 0x35, 0x35,
            0x48, 0x2d, 0x53, 0x86, 0x9c, 0xbe, 0x01, 0x41, 0x18, 0xc7, 0x4a, 0xfa,
            0xe3, 0x44, 0x8a, 0x47, 0x02, 0xb0, 0x98, 0x93, 0x1e, 0xea, 0x64, 0x5d,
            0x48, 0x9e, 0xf9, 0xad, 0xc6, 0x07, 0xbb, 0x9f, 0x9e, 0xb5, 0xa8, 0xce,
            0x59, 0x68, 0xcc, 0xb3, 0x8b, 0xd8, 0x42, 0xd7, 0x30, 0x7d, 0x51, 0x77,
            0xa0, 0x47, 0x7d, 0xfe, 0x96, 0x63, 0x79, 0xaf, 0x31, 0xe8, 0xfb, 0xa1,
            0x9c, 0xce, 0x74, 0xf8, 0x53, 0xe9, 0x9e, 0xf8, 0x2e, 0xe7, 0xb2, 0xfc,
            0xbc, 0x77, 0x5e, 0x5a, 0x30, 0xaa, 0x02, 0x23, 0xf2, 0xa2, 0x64, 0x12,
            0x2e, 0x02, 0x09, 0x3e, 0x3b, 0xcc, 0x27, 0x0e, 0x1a, 0x1d, 0x45, 0x04,
            0xde, 0xd1, 0xd7, 0xd5, 0x63, 0x81, 0xf1, 0x70, 0xc7, 0x76, 0xfb, 0xa1,
            0x8b, 0xd8, 0x19, 0x38, 0x3c, 0xd9, 0x59, 0xb7, 0x48, 0xf7, 0x62, 0x4d,
            0x86, 0x82, 0xdb, 0xc2, 0xf8, 0xe6, 0x84, 0xa4, 0xde, 0x4c, 0xdf, 0x85,
            0x6c, 0x1b, 0x5d, 0x24, 0x88, 0xc2, 0xd3, 0xe5, 0x74, 0xb0, 0xc1, 0x66,
            0x18, 0xd2, 0x50, 0xf1, 0x62, 0x24, 0x26, 0xc7, 0x0d, 0xd2, 0x1c, 0xd1,
            0x92, 0x67, 0x65, 0x53, 0xde, 0xb9, 0x94, 0x77, 0xb8, 0x86, 0xa9, 0x5c,
            0x33, 0x21, 0x1d, 0x4d, 0x29, 0xd8, 0x4a, 0x70, 0xbb, 0x34, 0x81, 0xc5,
            0x7e, 0xf2, 0x45, 0x04, 0x16, 0x22, 0xf6, 0x47, 0x00, 0x54, 0x1c, 0x24,
            0x63, 0x20, 0x0b, 0x40, 0x3b, 0xfa, 0x89, 0x31, 0x8c, 0x48, 0x8d, 0x75,
            0xfe, 0x77, 0x30, 0x75, 0x3c, 0xf8, 0xfc, 0x61, 0x8f, 0xa1, 0xd6, 0x94,
            0x3c, 0x5c, 0x5f, 0xfd, 0x65, 0x47, 0x53, 0x3e, 0xab, 0x04, 0x0b, 0x53,
            0x2a, 0xf9, 0xfc, 0x2a, 0x0e, 0xb5, 0x17, 0xa7, 0x15, 0x85, 0xc1, 0x52,
            0x0a, 0x38, 0xb0, 0xd9, 0xb5, 0x86, 0xa5, 0xe5, 0x9c, 0x59, 0x32, 0xfe,
            0x08, 0x66, 0xdf, 0x60, 0x2b, 0xd9, 0x36, 0x20, 0x15, 0xc8, 0xee, 0xd0,
            0x76, 0x67, 0xe3, 0xe6, 0x1a, 0x0b, 0xbb, 0xb2, 0x86, 0xfa, 0x7c, 0x41,
            0x2c, 0x5f, 0x2e, 0xa1, 0x49, 0x7a, 0xb7, 0x70, 0x5d, 0x2b, 0x3f, 0x43,
            0xaa, 0x0f, 0x55, 0x7f, 0xe4, 0x15, 0xb5, 0xa5, 0x63, 0xfc, 0x23, 0x83,
            0xd6, 0x75, 0x32, 0x91, 0x75, 0xd1, 0x57, 0x17, 0xcb, 0xa3, 0x87, 0x4d,
            0x05, 0x2f, 0x9a, 0x56, 0xc9, 0x8e, 0x16, 0x3a, 0x69, 0x29, 0xa1, 0x2f,
            0x99, 0xe4, 0xf9, 0x4b, 0x05, 0x8e, 0x49, 0x5a, 0xca, 0x67, 0x03, 0xee,
            0x4e, 0x29, 0x55, 0xf6, 0x30, 0x9e, 0x84, 0x56, 0xc5, 0x64, 0x5e, 0x8c,
            0x3f, 0x28, 0x96, 0xe6, 0x67, 0x29, 0xf6, 0xde, 0x03, 0xdb, 0xb8, 0x4c,
            0xa3, 0x3f, 0xd2, 0x88, 0xa8, 0xc2, 0xa9, 0xd3, 0x7d, 0xdd, 0x87, 0x41,
            0x3b, 0x81, 0x41, 0x17, 0xf7, 0x16, 0x0f, 0x95, 0x87, 0x98, 0x3e, 0xca,
            0x69, 0xa2, 0x58, 0x9a, 0x7c, 0xed, 0xcf, 0x75, 0xbe, 0xc5, 0x2e, 0xdb,
            0xfe, 0xa9, 0x36, 0x3d, 0xc3, 0x7e, 0xaa, 0x96, 0x44, 0x5e, 0xc7, 0xeb,
            0x5a, 0x27, 0xb8, 0xa6, 0xb0, 0x75, 0xba, 0x61, 0x12, 0x6a, 0xf6, 0x0b,
            0x0b, 0x5a, 0x4b, 0x86, 0x81, 0x20, 0x73, 0x48, 0xd1, 0x52, 0x5d, 0x5b,
            0xf6, 0x61, 0xd2, 0x63, 0xb1, 0x38, 0xa6, 0xe9
    };
    const unsigned int signature_sha256_len = 512;

    const std::string signature
    {
        "Oo8ZlDQcMlZF4hqnhN/2Dz3FYarZHrGf+87i+JUSxBu2GKFk8SYcDrwjc0DuhUCx"
        "pRVQppMk5fjZtJ3r6I9066jcEpJPljU1SC1Thpy+AUEYx0r640SKRwKwmJMe6mRd"
        "SJ75rcYHu5+etajOWWjMs4vYQtcwfVF3oEd9/pZjea8x6PuhnM50+FPpnvgu57L8"
        "vHdeWjCqAiPyomQSLgIJPjvMJw4aHUUE3tHX1WOB8XDHdvuhi9gZODzZWbdI92JN"
        "hoLbwvjmhKTeTN+FbBtdJIjC0+V0sMFmGNJQ8WIkJscN0hzRkmdlU965lHe4hqlc"
        "MyEdTSnYSnC7NIHFfvJFBBYi9kcAVBwkYyALQDv6iTGMSI11/ncwdTz4/GGPodaU"
        "PFxf/WVHUz6rBAtTKvn8Kg61F6cVhcFSCjiw2bWGpeWcWTL+CGbfYCvZNiAVyO7Q"
        "dmfj5hoLu7KG+nxBLF8uoUl6t3BdKz9Dqg9Vf+QVtaVj/COD1nUykXXRVxfLo4dN"
        "BS+aVsmOFjppKaEvmeT5SwWOSVrKZwPuTilV9jCehFbFZF6MPyiW5mcp9t4D27hM"
        "oz/SiKjCqdN93YdBO4FBF/cWD5WHmD7KaaJYmnztz3W+xS7b/qk2PcN+qpZEXsfr"
        "Wie4prB1umESavYLC1pLhoEgc0jRUl1b9mHSY7E4puk="
    };


    const std::string temporary_public_pem
    {
        "-----BEGIN PUBLIC KEY-----\n"
        "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA3Delbh36s+NJCCOYi1ql\n"
        "NGp+R5EoKWtcazi+Kh/t2V4kN4QCEQdxi3nlhbHdiWWNi8puwwJYFDRdjZvEO+2H\n"
        "yEyFui4v9Rl/RoGdDQeXEeQ+QxMVvT6Ya7+unRDjlIuNmQNNe4HKlcA4fqhRqi07\n"
        "bF2b1kceuPsxIXcBnrsVVqxvSvqjiVkaSnk+HACm8fjiTwGSFE3ZhooMgxENEWrw\n"
        "Ltcr80UdWpMvCrlBCWtlxiLl8VgoPCDZ/R2iXvJaQAOwepfdFGmcPomcO9BOEJfn\n"
        "Jg4sBQNu4CqN5KRCS8CN39E705s4upFMRleU6nKHa0kSb9OsqJP/0i0fBVFLsxpX\n"
        "WbR2iLwpCy3YqJqwoS8PFz9rr4V9ESqRJlczjkRt6bfx4/fSskxsXWRl+Dlv1V/P\n"
        "Sl0zlJXMoxpPuLANxEVsnR9fT07h0XuQ58dsjbMImTL8Hmomfl3n2WK35TesAAaO\n"
        "WNa4+2LFYzKwLUL9cRv696544eo8TCi+bXEKSCmUzpLsLIeHVmeHy3CiUUq13ktE\n"
        "ykQD9vdtoyJkWJ4n5QMQVaV1k1hJ0V8EMfpV9mMEpItfQQRqDW1QG2fHbZgrUV1m\n"
        "5PdnWwf3L9nMNxja7nX0vk5QVw1r8Kl4KqP4sid4djBYz4SBLeSfimUDw/USsKik\n"
        "MwoaYGgrejBmlvZ5UiFw+xMCAwEAAQ==\n"
        "-----END PUBLIC KEY-----"
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


TEST(util_test, test_that_verifying_a_signature_with_empty_inputs_will_fail_gracefully)
{
    EXPECT_FALSE(bzn::utils::crypto::verify_signature( "", signature, valid_uuid));
    EXPECT_FALSE(bzn::utils::crypto::verify_signature( public_pem, "", valid_uuid));
    EXPECT_FALSE(bzn::utils::crypto::verify_signature( public_pem, signature, ""));
}


TEST(util_test, DISABLED_test_that_ethereum_will_provide_bluzelle_public_key)
{
    EXPECT_EQ(temporary_public_pem, bzn::utils::crypto::retrieve_bluzelle_public_key_from_contract());
}
