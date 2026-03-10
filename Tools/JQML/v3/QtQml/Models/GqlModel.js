const TreeItemModel = require("./TreeItemModel")

class GqlModel extends TreeItemModel {
    SetGlobalAccessToken(token){
		XMLHttpRequest.QMLAuthToken = token
	}
	
	SetRefreshToken(token){
		XMLHttpRequest.QMLAuthRefreshToken = token
	}
    
    setGqlQuery(gqlData, headers){
        this.state = "Loading"
        let xhr = new XMLHttpRequest
        xhr.open("POST", "../../graphql")

        if(headers && typeof headers === 'object'){
            for(let name in headers){
                xhr.setRequestHeader(name, headers[name])
            }
        }

        xhr.onreadystatechange = ()=>{
            if (!this.__destroyed && xhr.readyState === XMLHttpRequest.DONE){
				if (xhr.status === 401){
					this.state = "Unauthorized"
				}
				
				if (xhr.status === 403){
					this.state = "Forbidden"
				}
				
				if (xhr.status === 200){
					this.json = xhr.responseText
					this.state = "Processing"
					this.updateJSONModel()
					this.updateTreeItemJSONModel()
					this.state = "Ready"
				}
			}
        }

        xhr.send(gqlData)
    }
}



module.exports = GqlModel