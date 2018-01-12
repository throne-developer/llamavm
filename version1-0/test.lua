
function member(name, score) 
  return {["name"]=name, ["score"]=score}
end

function output(members)
  local i,len = 1, getn(members)
  while i <= len do
    print(members[i].name, members[i].score)
	i=i+1
  end
end

function sort(members) 
  local i,len = 1, getn(members)
  while i<len do
    local j=1
	while j<=len-i do
	  if members[j].score < members[j+1].score then
	    members[j],members[j+1] = members[j+1],members[j]
	  end
	  j=j+1
	end
	i=i+1
  end
end

members = {
  member("lily", 80),
  member("tom", 90),
  member("adam", 100),
  member("baby", 75),
}

output(members)
print("after sort")
sort(members)
output(members)