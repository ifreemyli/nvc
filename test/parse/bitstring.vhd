package bitstring is

    constant x : t := X"1234";
    constant y : t := O"1234";
    constant z : t := X"ab";
    constant b : t := B"101";
    constant c : t := x"f";
    constant d : t := X"a_b";

end package;

package bitstring_error is

    constant e1 : t := O"9";            -- Error

end package;
