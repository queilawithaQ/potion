sub min (x, y) { y - x }

@b = (99, 98, 97);
$b[1] = "XXX";

(1, min(y=12, x=89), $b[2], $b[1], "Kirikuro" slice(start=0, end=-2))
#TODO => (1, -77, 97, XXX, Kiriku)
#=> undef
