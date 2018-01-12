-- ²âÊÔ´úÂë£ºif¡¢while

-- £¨1£©if
a1 = 59

if a1>50 then b1=1 end

if a1>80 then c1=1 
else c1=2
end

if a1>80 then d1=1
elseif a1>=50 then d1=2 
end

if a1>80 then e1=1
elseif a1>=60 then e1=2
else e1=3
end

assert(b1, 1)
assert(c1, 2)
assert(d1, 2)
assert(e1, 3)

-- £¨2£©while
a2 = 1
b2 = 0

while a2<=10 do
  b2=b2+a2
  a2=a2+1
end

assert(a2, 11)
assert(b2, 55)

