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
#include "../is_whitelist_member.hpp"
#include "../crypto.hpp"


using namespace::testing;


namespace
{
    const char* WHITELISTED_UUID    {"12345678-0900-0000-0000-000000000000"};
    const char* NOT_WHITELISTED_UUID{"e81ca538-a222-4add-8cb8-065c8b65a391"};

    // NOTE - not constant as the test shuffles the list before use to get five
    // random uuids.
    std::vector<std::string> whitelisted_uuids = {
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

    const std::string private_pem{
            "-----BEGIN RSA PRIVATE KEY-----\nProc-Type: 4,ENCRYPTED\n"
            "DEK-Info: AES-128-CBC,85B7C2BF5A331057198359FFC062CA9D\n"
            "\n"
            "b/05ogEkNDTDm9KI0mIHu1gqHh0Mh9FXDxMq5nNcrgq0/fM7Fm95qA5Dxsn0Zckd\n"
            "QuCHFOtd5GuJi0y4em5AoXMYLX85AQDbPV9gct93Cph+bhEZHMknIDbciKEXmSpe\n"
            "NE6zffCfb1L0NmFgZMomv6jMOB1c2QnwcIcte1S5w+r0Wr+tmxQkfWJwmCY3Cebg\n"
            "byTI/PuHXD6bZYQmdunCw4h1+R2c63yEfnW2tvSau29B5965fxHB1nLGolHBNN/w\n"
            "82tCWZre/e9UTGURftSlSDsI1wXD3uJB7qFJM/zwZ9UJ1X8CnyC+UNkFgbpvnktk\n"
            "xZKARY/D2b1/X6vmgL8pAYQDHW+wK4i9xG2p9HDzKRqJOQvEvO3DNxpGqgpuWIV4\n"
            "ri2NSeWnAxB8jgUl7wSML9EwjfNcK+Xf45aFn2vIhzhUzIF80a7AO6u/yrNGZZ8Z\n"
            "nH0ICr3P/7a9WSoH0mvM1flMPMwh92Bz9e6xdNYZ7dsSjCC9JHy4PRRJJlWvukbb\n"
            "7MDSOur6Jkxl0LlM2G9hKM/Zy6QD6Kq/zFAiwzO8Ql0Zd0yZDKiTermcF21ic/oV\n"
            "A5ULnekYL4NaBFIiq/QmzC8rVdhjvyYFGZzOnFRSYCzfE0BQVrhgb3kQqaaU+/xR\n"
            "aSxMCUMMSY6qKjTrkRP+5TCnFFo7LVrVo24gPKfqn9Ts6QvcPbEhoq/hOd4z/iy0\n"
            "Byl++WI8ZEXoxI+4J7INY9VA0iLRcTo7ZwKAUPiH/rjWA2VVNKWc8xPVa/dUrHpO\n"
            "miU/5QYBpQJLWZVoO/cO1HpeVFMkdTr8rHMM7WlnQmmszZoIME1WFmqLyWecQukw\n"
            "vx7RtiTcA8Nk4HpgCfWebaHevPsiSKk+NV2l3Ja65765u943r8ZmqwqliPZUDAEL\n"
            "/se5ohP1L7V1IvS8iSGPkTqWzCFzvjA/sI8m2CNWiWYMOY05IATH/293gzx8SvLf\n"
            "gqgDEkAXIAyI2MxTQobfHhV9K8oGP47oPmluW4wHNty9vBGcaWdvIBqYdVIgSGMN\n"
            "uR5OkunuSTY1IccCulfOO4GKtmm1FJxn36Tg0MwB4bxm2zjz0D/d4WP2xh7kz+6C\n"
            "ayOgLa4fdiuFjdqtYq2he8v6JPkA1DGLqKzGaYd/n1yQ0ee0g3EDbxhrcsq26ilX\n"
            "zMM4CMRwDAimllJH/rX5p4lsb0XMA2eUvOLVW0Kt3TEpboaleAXvgrhB6zEfpIcw\n"
            "c0al74e/p5gogKz/9gs7QpWMuvU3+wGgqkAMZohFTEvnwz2BPVxgSMsdRnlqoAxs\n"
            "1h5+cmrvPI3DCof+Sdqg9F6VzGcUH86HnoEEHN9dQk956i6seqjqguHEDHX+IXK3\n"
            "uQO0wt76Di/flT/eI/cX2TQ7Ut/5S0HZHD8xU58FeT7VEl99IuejlfG5dFMmsi3Z\n"
            "Kf0OdfWMRDiwWPPEfuvX6wpwP8lE4LdpTcjRN1Z+JjdhgBC1OsUyWMx8h85l+nzu\n"
            "88GabwE0+HMxM8kp7yQXQlwJP6MxaxWoEptOL5GkC2YYpLQvLYZGTvlfV0ARNh1L\n"
            "Mqkhbc/TXfe0ai5cH2C1+RUceUvQtUz5a2PFah9kLbmfhmVIstA+Dg2rBAf4xVLE\n"
            "LUMfres8rngIcxXW6hFMIdYKsawTo+Y1c9ro/gV99DWQdXHzO+6Ihc981Yk2fR2B\n"
            "Gr5wezbkPvjzQRByWw0WMGJ6lfz57vLbdABqbRce/E64zITuEXaEHtLBHlAAb2UM\n"
            "5bWLxoqBGntGZgiK0BfvDft6bimaA1EPEsLR15OMjn6OdMmx8gBSUlggXpQHwrty\n"
            "3d8h/gOlh2i12mFQCrujgo6eLIZkACe8it1PTSEbkT1Qe2QH9Iw4Pgc+IodNQ9Ca\n"
            "de13bC0AhXsCTJ348CzglbW5/zzCzBpL/ghXfUoYFoddTvkxKVDoSTaTeB8DIJ+g\n"
            "M0qOb+fUJMioqxC1ptofsip+maIQN4LUDdrPLAoFeT6ENqP65UvV3nWro8X7/D9P\n"
            "Ik9gC0adgbQoS47MaH6jNqBpL+xxq7TnZxkrd+Rrqexd8KW0GGVAVzRaByJ/GEAo\n"
            "/5ehFJYrx2fefLzbykBOWc+wclOri8EdC1hALVg++Rz9UFx5V9xoEd7zX4KSl6WQ\n"
            "19XEnJEmxmOSLRQ4CcNBByUTOxmlGN+rcnVJGTDaiA+Ybrfme5n00FPte5zA1jZ4\n"
            "A06i9+vpCJS71qNKtj2zGmMLkoXYM2BxHKvKsvmIcET2F4oBuq8Oqe93FQOsz4HC\n"
            "lr9+kHWhWmcKnZV63CXg63soX1nRQhxPZa8HoCa7ptK9nCE2n2xbMKbCvOwBMyIL\n"
            "PoiPrxJmWxXHJ8IfFCUVaVMpKMagtKcSatD9ZYE9aAxM9389CUZGM6B0gXS80dRg\n"
            "9QWiZf5ub+fJC7diELIb4vjHAyZ/K+b5VAJO8dp5MhAmXkV3vePZgzHmaK38gTJp\n"
            "KfOetBMmvlqFfnTe2oz0VM15qtu/pnwIG1jwFEsU5I17ByAUFm3GjcQ1Moa7iTWN\n"
            "06aAJtr3y15CHI3/GkvPSfvKEYG41+hkzUQoSacOiX7Gj2e6qIQbx/Bfs5Pqo90J\n"
            "/BSjA6y/kAFXDiAuKJhMjNUzQNx9Z408tfjUUiAoRs5/VVWqDho4oKrcw5Tpr6Nu\n"
            "8zow305aVHS5SD3YtRCPdMku+5V1cSR7+4KxXfJ6L76xR2GgJ7eZ4ncZqqAM3avT\n"
            "JrYGROLRDQFbUDqBkvIqyHRGqSXcpP+DSYpb5TOsIOaXYMdBjVWZOeHv8dYJIa8n\n"
            "BwrhGNgmRi25ftiHIOAOH9l74awKdhZfw1QTbii7D1lNlfkimsPpHNREvyPU21DA\n"
            "1wI3imCKEQFfBMxnHll+Ro91BcOHp9/7JhJzeDgolrSOeCu2BIuCytkjzWQC2sXE\n"
            "QXqiOosfyK9FQqK1+Pp2viNZI+s/UsZ3VBZxW+/qYihbQlXfrgeLsHEAo5tORcwC\n"
            "loPKCF766nk/s5pBaOc0cSO25+HLTZdGte00FekP6rjH9ZyAqPypLI14W3asMAAI\n"
            "TIdIAkX6N6O3UYhVejScZ7/Q7A1eDaXI1sAjrfFHxfpjnd34seJIQhTEAPK2CT0w\n"
            "hKrrboOl/DywQ7Dv0GFNqu76TDh8Re1RnnT7a4o7jgQhgCqthfu2jb67HX8FsfNT\n"
            "-----END RSA PRIVATE KEY-----"
    };

    const std::string public_pem {
        "-----BEGIN PUBLIC KEY-----\n"
        "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAsEI5VRZO8rzXkuoy8Jpn\n"
        "CcjYsmTk4DlNKqVJ6SeuBbQpyg+J2CZL4xH4IyhLhuwE5MklCRxFrWjqrYyaIih7\n"
        "+eTeaBGtmLdGuHyPw/cBcGWgU4twJjPZQo3WWPwovRkrJkmh7BR5VJ5VdiuwruuH\n"
        "ichYJBk+FSwGsdcryc33ujtNrWs46/wtc7skh6YiTo3svOH8HTm10vE+fasXS/r9\n"
        "T6AXBRpVUdlU2Bg+IIU6x/AUW6k0XYOD8K9dI5U3Rql4q+3qDGGWamfX9tTmu2xD\n"
        "YMyRPDp1rbzG2UKQAvMAdO9daxbnOGyV4Acr5tmD8k/bayQHYN3nsYapZb81/xqv\n"
        "474PJ62b/s3rauj1HAVRcZrrVEKwcxB1qBz6xD9xOHKunwOwjcDCmk3Hu+Mfc1ih\n"
        "izJexxxETjaF0YzrhSFw0h+cqHvacWe38jYp3iL4DI/h4u6U96pNobWDs9TTNld/\n"
        "62LCbw3tZMNtQ04sM6kuVU/XfKzvpjs77jswYHzs/ZJfWyv8DdK3syo2ieZsMlQn\n"
        "RfqfN2hfyfiSxAxErhfvSbQVT/2LIIerm1O5/F3H3383TyLc09h1AMGoflQTa4Oo\n"
        "Bg8CYfuidf5kA+He2oQy+HXF4vxLyDwuEoU2CPI8KJgY0VXlPwX20B+QOEoM/M9c\n"
        "sSsuY0Gg7UWXIiV5BIFrdssCAwEAAQ==\n"
        "-----END PUBLIC KEY-----"
    };

    const std::string valid_uuid        {"9dc2f619-2e77-49f7-9b20-5b55fd87ea44"};
    const std::string invalid_uuid      {"8eb1e708-2e77-49f7-9b20-5b55fd87ea44"};

    const std::string signature {
            "OFww+6HZgexU9nMeHY3j/O+jgqkWZK03JwgZLLdeeW7WFfDBYPWheoTH+5s7DQfd"
            "rJoSpW4lttFYe01xkJbt7kdqaO80mt2v5jW9QdkN4HOTRUryKkvsIkG91fyieqO7"
            "ruXXVvBzteCeuMOYI37AKJbCyEtftLC443Kk1WtJ38KyTdMbpmRf5mz5VpCIyFAQ"
            "UXTOhw+Yo3RQO2ZiwMbfkRoNSXWQxf3Ajo6fmxwQggBIYNivTuIA1sCZedZp/Lxw"
            "/WOC6MBlXUJybhLLWlKdH0M+CdNhENxcnGM7csezfw/0Gvbfj/gz5b2re2xu5hV0"
            "OQfLFCVhntNj+l9shjWq6wfH1ExGrsi24qq6I5QTBEE568ZpV0Bu0uAb5tcUY+LM"
            "gVDtMeF71sigm9D4e5n8PRDg9//E/RdyiEMvXRaLqPXL+QRiNtbM1jBK+USPa0Mz"
            "cB49hS9uKsghXgfT5kz+cKNWXPxSxopTblSrkf7n4yZGbkzxa3ZkiD/Jp5fNdLnn"
            "WVeUHrU7w1TZ3LGpuok+cbNn3TWfZsCUJHkNOm0EHYeXVCJY3thwMbgSL8AuKkfz"
            "wvFHVqbxu2DnPUjqhUt8dKp0RXo8CQpQQWbFitV4Eif9kHyIMoew8oM9GXip9tLJ"
            "4bJW3WqEujHG3deW7utQbPse63pK20PmaPZIBWuO7lQ="
    };



}


TEST(util_test, test_that_is_whitelist_member_returns_TRUE_if_uuid_IS_found_in_whitelist)
{
    EXPECT_TRUE(bzn::is_whitelist_member(WHITELISTED_UUID));

    std::shuffle(whitelisted_uuids.begin(), whitelisted_uuids.end(), random_engine );

    std::for_each(whitelisted_uuids.begin(), whitelisted_uuids.begin()+5,[](const auto& uuid) {
        EXPECT_TRUE(bzn::is_whitelist_member(uuid));
    });
}


TEST(util_test, test_that_is_whitelist_member_returns_FALSE_if_uuid_is_NOT_found_in_whitelist)
{
    EXPECT_FALSE(bzn::is_whitelist_member(NOT_WHITELISTED_UUID));
}


TEST(util_test, test_that_a_poorly_formed_uuid_fails)
{
    EXPECT_THROW(bzn::is_whitelist_member("0}56fcb3-ae1e-4b3e-b794-be3270cc9d43", "http://localhost:74858"), std::runtime_error);
}

TEST(util_test, test_that_a_uuid_can_be_validated)
{
    EXPECT_TRUE(verify_signature( public_pem, signature, valid_uuid));
    //EXPECT_FALSE(verify_signature( public_pem, signature, invalid_uuid));
}

