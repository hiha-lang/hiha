/*

  The hiha programming language

  Copyright © 2026 Barry Schwartz

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <config.h>
#include <inttypes.h>
#include <gc/gc.h>
#include <libhiha/libhiha.h>

/*
  A simple check of string_t_hash() to at least gain some assurance
  that it does not crash and returns reproducible numbers. The number
  also must differ, depending on index.
*/

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

static const char *const kind_str = "PROVERB";

/* Do not be afraid of a barking dog. Be afraid of a silent dog. */
static const char *const proverb_str =
  "Ne timu hundon bojantan. Timu hundon silentan.";

const uint64_t nominal[] = {
  0xE5C3E40E87BF884A,
  0x7286A08F9CEB802E,
  0x59F6A4DDD4534391,
  0x7206680C831CD340,
  0x25A4253F234DED9B,
  0x10E7B752FCC12983,
  0xDFD81BF421BFF348,
  0x93623501A98190DA,
  0x7E58C0AA05A5C7F4,
  0x13B1766D8FDB10CA,
  0x62B15EB9FC20B603,
  0xED0F9D925EC9ECD9,
  0x8D4F5E9D51E206A9,
  0x6DCA69DB313F59FD,
  0xAE1FEB71E5E46BEC,
  0x6FD0759923713D4E,
  0x883F50536CA83A88,
  0xAE88032225EC4219,
  0xBF321E25DD53FC7E,
  0xE732E5B4AE4477CB,
  0xA9AD11BABF9A75B5,
  0x5E4A59BAE0FFDC8C,
  0x0FF5EC291A078EFF,
  0x5EF869FDBA983459,
  0x8882E6FCE73FD7AA,
  0x9C3111C22797F44E,
  0x20A59BC6A8E23028,
  0xAC7BF5091C07B22D,
  0x467C5FCACC2C39A0,
  0xE2BAFE08EF093B0A,
  0xE827B4C88EB99D6A,
  0x494B05CF03794426,
  0x9F6E462312682ACE,
  0xD0EFA64BB026A117,
  0xCBA1EA7CC312F95D,
  0x2A1B9E2EC55BD5ED,
  0xF480B586D60F4224,
  0xBA296CE74680C61A,
  0xFF276F8672E371B9,
  0x5DDD44A805165633,
  0x27CC2A302822057C,
  0x8DFF67357168404B,
  0x8DF22A8B7AA44718,
  0x7B79A656DCE7D486,
  0xC7DD2090240320D8,
  0xDDDE0447C0CEE557,
  0x04B461A8865E5EDE,
  0x15C50156921E318C,
  0x763169E5C47A3F15,
  0xDFE05D3DFAAC0C4B,
  0x496311B2517276E6,
  0xCCFB8DFA49E9DB09,
  0x1D5604D07DCB3FA5,
  0x0E7646794D3BDADD,
  0x73ACB4B3D577D62E,
  0x63B24F782BDEBA12,
  0x8B6B0F74BD4FCCCE,
  0x9EA931C346D3662E,
  0x80DBDA5945118C67,
  0x3F184CECF06E25B2,
  0xC6F91632189F9897,
  0x27CA158B4D8EA073,
  0x3042479ACFE57127,
  0x05A8561AB340D6E1,
  0x151B67845EB0AFB9,
  0xCDBE53EB8A9FE3CA,
  0x20D65290F264180E,
  0xC746058F86B62802,
  0x8C39279CB01C7D7F,
  0xDE2AB54837334E3C,
  0x57B25613B89301A2,
  0x8A1BA29CA8687DFC,
  0x1EF3C02840EA6D7C,
  0x86C6408AD26ED2E2,
  0x343F5B99D452B6B4,
  0xA4C7FBCC1CEAA573,
  0x4B3F02F0477C7A91,
  0x21B090715D53505C,
  0x00C8EED985914E14,
  0x5BAD4EBF5338CC60,
  0xE296FE1D608F4FE9,
  0x6D1BDEA571185923,
  0x8AE949E116B7E612,
  0xE819F69BC5D2E226,
  0x00EC774E7440D3FA,
  0xA9FECA1FF84E0FCE,
  0xB532145568C00BD4,
  0x7312DDF05FB028AF,
  0xD2671017B08AD1EF,
  0x47198FAF9971AAC3,
  0xD433F769C86498EF,
  0x8147CF60A74F3D09,
  0xF48654C335715E64,
  0x146AF323173DE478,
  0x8F4A4901B40F5FF1,
  0x52E0370887B8D847,
  0xF010AA90485BDF1F,
  0x60A17CA431096AF9,
  0x9913057B34A37394,
  0xBB6A8E2B2FF93A44
};

int
main (void)
{
  GC_INIT ();

  const token_t tok = make_token_t (make_string_t (kind_str), make_string_t (proverb_str), NULL);

  token_t_hash_context_t context = token_t_hash_init (tok);
  for (size_t i = 0; i != 100; i += 1)
    {
      //printf ("0x%016" PRIX64 ",\n", token_t_hash (context, i));
      if (token_t_hash (context, i) != nominal[i])
        abort ();
    }

  return 0;
}
